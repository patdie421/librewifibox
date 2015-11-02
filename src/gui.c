#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // close
#include <inttypes.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#include "gui.h"

#include "mea_verbose.h"
#include "mea_ip_utils.h"
#include "mea_string_utils.h"
#include "mea_queue.h"

#include "mea_memfile.h"

#include "mongoose.h"
#include "params.h"
#include "iwlib.h"

static int reboot_flag=-1;

const char *str_document_root="document_root";
const char *str_listening_ports="listening_ports";
const char *str_index_files="index_files";
const char *str_num_threads="num_threads";

char *val_document_root=NULL;
char *val_listening_ports=NULL;
char *val_num_threads=NULL;
char *val_sslpem=NULL;
char *val_user_params_file=NULL;

char *titre="LibreWifi2";

// Variable globale privée
int  _guiServer_id=-1;
int  _guiServer_pid=-1;

const char *options[15];
struct mg_context* g_mongooseContext = 0;

int librewifi_pid=-1;

struct session_s
{
   char id[33];
   int logged_in;
   time_t first;
   time_t last;
   int counter;
};

mea_queue_t sessions;
pthread_rwlock_t sessions_rwlock = PTHREAD_RWLOCK_INITIALIZER;

int isinit_sessions=0;


static const char *_open_file_handler(const struct mg_connection *conn, const char *path, size_t *size)
{
   /*
    // ne peut pas marcher avec les cgi car le cgi-php va chercher sur disque le fichier ... utile néanmoins pour fournir des pages statiques ...
    char *content = "<html><head><title>Test PHP</title></head><body><p>Bonjour le monde</p></body></html>";
    
    int i;
    for(i=0; path[i]==val_document_root[i]; i++);
    
    if(strcmp(&path[i],"/test.html")==0)
    {
    *size = strlen(content);
    return content;
    }
    */
   return NULL;
}


char *http_header_common =
             "Cache-Control: no-cache, no-store, must-revalidate\r\n"
             "Pragma: no-cache\r\n"
             "Expires: -1\r\n"
             "Vary: *\r\n";


void _httpResponseSetSessionAndRedirect(struct mg_connection *conn, char *mea_session, char *where)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   mg_printf(conn,
             "HTTP/1.1 301 Moved Permanently\r\n"
             "Location: %s\r\n"
             "Set-Cookie: MEASESSID=%s\r\n"
             "%s" 
             "\r\n\r\n",
             where, mea_session,http_header_common);
}


void _httpResponseRedirect(struct mg_connection *conn, char *where)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   mg_printf(conn,
             "HTTP/1.1 301 Moved Permanently\r\n"
             "Location: %s\r\n"
             "%s"
             "\r\n\r\n",
             where, http_header_common);
}


void _httpResponse(struct mg_connection *conn, char *response)
{
   mg_printf(conn,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %d\r\n" // Always set Content-Length
             "%s"
             "\r\n"
             "%s\r\n",
             (int)strlen(response)+2, http_header_common, response);
}


char *new_id_session_alloc(int size)
{
   char *id_session = (char *)malloc(size+1);
   if(id_session == NULL)
      return NULL;

   int i;
   char c;
 
   double nt=mea_now();
   srandom((unsigned int)(floor((nt-floor(nt))*10000000)));
   for(i=0;i<size;)
   {
      int r=random();
      c=(char)(((float)r/(float)RAND_MAX)*256.0);
      if(isdigit(c) || isalpha(c))
      {
         id_session[i++]=c;
      }
   }
   id_session[i]=0;

   return id_session;
}


static struct session_s *find_session(mea_queue_t *sessions, char *session)
{
   struct session_s *e;
   if(mea_queue_first(sessions)==0)
   {
      while(mea_queue_current(sessions, (void **)&e)==0)
      {
         if(strcmp(session, e->id) == 0)
         {
            return e;
         }
         mea_queue_next(sessions);
      }
   }
   return NULL;
}


int page_reboot(struct mg_connection *conn, struct mea_memfile_s *mf, char *wifinetwork)
{
   mea_memfile_printf(
      mf,
      "<html>"
         "<head>"
            "<title>"
               "LibreWifi"
            "</title>");
   mea_memfile_include(mf, "/data/librewifi2/www/include/libs.inc", 0);
   mea_memfile_printf(
      mf,
	"</head>"
        "<body>"
           "<div style=\"min-width:950px;min-height:650px;margin:10px;text-align:center;\">"
              "<div style=\"display:inline-block;margin-top:75px\">"
                 "<div class=\"easyui-panel\" title=\"Rebooting...\" style=\"width:400px;padding:30px 70px 20px 70px;margin:0 auto\">"
                    "LibrewifiBox restarts to use your new parameters. You will have to reconnecte to %s wifi network in a few seconds."
                 "</div>"
              "</div>"
           "</div>"
        "</body>"
     "</html>", wifinetwork);

    _httpResponse(conn, mf->data);
}


static int page_index(struct mg_connection *conn, struct session_s *session)
{
   const char *cl;
   struct mea_memfile_s *mf=NULL;
   int error=0;
   char pass1[32]="", pass2[32]="";
   char essid_name[IW_ESSID_MAX_SIZE+1]="", essid_pass[64]="";
   char ip[32]="", netmask[32]="", dhcp_start[32]="", dhcp_end[32]="";
   char ip_netmask[sizeof(ip)+sizeof(netmask)];
   char dhcp_start_end[sizeof(dhcp_start)+sizeof(dhcp_end)];
   char freewifi_login[21]="", freewifi_pass[33]="";
   char submited[2]="";

   if(session->logged_in==0)
   {
      _httpResponseRedirect(conn, "/login.html");
      goto page_index_clean_exit;
   }

//   struct cfgfile_params_s *params = mea_cfgfile_params_alloc(user_params_keys_list);
   struct cfgfile_params_s *params = mea_cfgfile_params_from_string_alloc(get_user_params_keys_list_str());
   if(params==NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't alloc user configuration file\n", ERROR_STR, __func__);
      _httpResponse(conn, "Internal error");
      goto page_index_clean_exit;
   }

   int ret=mea_cfgfile_load(val_user_params_file, params);
   if(ret<0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : can't load parameters file (%s)\n", ERROR_STR, __func__, val_user_params_file);
      _httpResponse(conn, "Internal error");
      goto page_index_clean_exit;
   }

   mf=mea_memfile_init(mea_memfile_alloc(),AUTOEXTEND,1024,1);
   if(mf==NULL)
   {
      _httpResponse(conn, "Internal error");
      goto page_index_clean_exit;
   }

   int issubmit=-1;
   if ((cl = mg_get_header(conn, "Content-Length")) != NULL)
   {
      char *buf;
      int len = atoi(cl);
      if ((buf = malloc(len)) != NULL)
      {
         mg_read(conn, buf, len);

         mg_get_var(buf, len, "submited", submited, sizeof(submited));

         if(strcmp(submited,"1")==0)
         {
            issubmit=0;
            mg_get_var(buf, len, "pass1", pass1, sizeof(pass1));
            mg_get_var(buf, len, "pass2", pass2, sizeof(pass2));
            mg_get_var(buf, len, "essid_pass", essid_pass, sizeof(essid_pass));
            mg_get_var(buf, len, "essid_name", essid_name, sizeof(essid_name));
            mg_get_var(buf, len, "fwpass", freewifi_pass, sizeof(freewifi_pass));
            mg_get_var(buf, len, "fwuser", freewifi_login, sizeof(freewifi_login));
            mg_get_var(buf, len, "ipbox1", ip, sizeof(ip));
            mg_get_var(buf, len, "netmask1", netmask, sizeof(netmask));
            mg_get_var(buf, len, "ipstart1", dhcp_start, sizeof(dhcp_start));
            mg_get_var(buf, len, "ipend1", dhcp_end, sizeof(dhcp_end));
            sprintf(ip_netmask,"%s, %s", ip, netmask);
            sprintf(dhcp_start_end,"%s, %s", dhcp_start, dhcp_end);

            if(strcmp(pass1,pass2)!=0)
               error=error | 0b00000001;

            if(strlen(essid_pass)<8 || strlen(essid_pass)>63)
               error=error | 0b00000010;

            int i=0;
            for(;essid_pass[i];i++)
            {
               if(essid_pass[i]<0x20 || essid_pass[i]>0x7E)
               {
                  error=error | 0b00000100;
                  break;
               }
            }

            if(strlen(essid_name)<1 || strlen(essid_name)>32)
               error=error | 0b00001000;

            if(strlen(freewifi_pass)<1 || strlen(freewifi_pass)>32)
               error=error | 0b00010000;

            if(strlen(freewifi_login)<1 || strlen(freewifi_login)>20)
               error=error | 0b00100000;

            uint32_t _addr, _netmask, _start, _end;
            if(mea_strisvalidaddr(ip, &_addr)==-1)
               error=error | 0b01000000;

            if(mea_strisvalidnetmask(netmask, &_netmask)==-1)
               error=error | 0b10000000;

            if(mea_strisvalidaddr(dhcp_start, &_start)==-1)
               error=error | 0b0000000100000000;
            else if(mea_areaddrsinsamenetwork(_addr, _start, _netmask)==-1)
               error=error | 0b0000010000000000;

            if(mea_strisvalidaddr(dhcp_end, &_end)==-1)
               error=error | 0b0000001000000000;
            else if(mea_areaddrsinsamenetwork(_addr, _end, _netmask)==-1)
               error=error | 0b0000100000000000;

            if( _addr != 0 && _start != 0 && _end !=0 )
               if( _addr >= _start && _addr <= _end)
                  error=error | 0b0001000000000000;

            uint32_t __netmask = ~_netmask;
            if( ( error & 0b01000000 & 0b10000000 ) == 0 )
               if( (_addr & __netmask) == (0xFFFFFFFF & __netmask) )
                  error=error | 0b0010000000000000;
          
            if(error==0)
            {
               struct cfgfile_params_s *new_params;

//               new_params = mea_cfgfile_params_alloc(user_params_keys_list);
               new_params = mea_cfgfile_params_from_string_alloc(get_user_params_keys_list_str());
               if(new_params == NULL)
               {
                  _httpResponse(conn, "Internal error");
                  free(buf);
                  goto page_index_clean_exit;                  
               }
 
               mea_cfgfile_set_value_by_id(new_params, ADMIN_PASSWORD_ID, pass1);
               mea_cfgfile_set_value_by_id(new_params, MY_ESSID_ID, essid_name);
               mea_cfgfile_set_value_by_id(new_params, MY_PASSWORD_ID, essid_pass);
               mea_cfgfile_set_value_by_id(new_params, FREE_ID_ID, freewifi_login);
               mea_cfgfile_set_value_by_id(new_params, FREE_PASSWORD_ID, freewifi_pass);
               mea_cfgfile_set_value_by_id(new_params, MY_IP_AND_NETMASK_ID, ip_netmask);
               mea_cfgfile_set_value_by_id(new_params, MY_DHCP_RANGE_ID, dhcp_start_end);

               int ret=mea_cfgfile_write(val_user_params_file, new_params);
               mea_cfgfile_clean(new_params);
               free(new_params);
               new_params = NULL;
               if(ret<0)
               {
                  _httpResponse(conn, "Internal error");
                  free(buf);
                  goto page_index_clean_exit;                  
               }
 
               int ppid=getppid();
               if(ppid!=1)
               {
                  kill(ppid, SIGHUP);
                  for(;;)
                  {
                     if(reboot_flag==0)
                     {
                        reboot_flag=-1;
                        break;
                     }
                     else if(reboot_flag==1)
                     {
                        page_reboot(conn, mf, essid_name);
                        free(buf);
                        goto page_index_clean_exit;                  
                     }
                     usleep(1000*500);
                  }
               }
            }
         }
         free(buf);
      }
      else
      {
         _httpResponse(conn, "Internal error");
         goto page_index_clean_exit;
      }
   }

   if(issubmit==-1)
   {
      int n;
      strcpy(pass1, mea_cfgfile_get_value_by_id(params, ADMIN_PASSWORD_ID));
      strcpy(pass2, mea_cfgfile_get_value_by_id(params, ADMIN_PASSWORD_ID));
      strcpy(essid_name, mea_cfgfile_get_value_by_id(params, MY_ESSID_ID));
      strcpy(essid_pass, mea_cfgfile_get_value_by_id(params, MY_PASSWORD_ID));
      strcpy(freewifi_login, mea_cfgfile_get_value_by_id(params, FREE_ID_ID));
      strcpy(freewifi_pass, mea_cfgfile_get_value_by_id(params, FREE_PASSWORD_ID));

      sscanf(mea_cfgfile_get_value_by_id(params, MY_IP_AND_NETMASK_ID),"%32[^,],%32[^/n]%n", ip, netmask, &n);
      mea_strtrim2(ip);
      mea_strtrim2(netmask);

      sscanf(mea_cfgfile_get_value_by_id(params, MY_DHCP_RANGE_ID),"%32[^,],%32[^/n]%n", dhcp_start, dhcp_end, &n);
      mea_strtrim2(dhcp_start);
      mea_strtrim2(dhcp_end);
   }

   mea_memfile_printf(
      mf,
      "<html>"
         "<head>"
            "<title>"
               "LibreWifi"
            "</title>"
   );
   mea_memfile_include(mf, "/data/librewifi2/www/include/libs.inc", 0);

   mea_memfile_printf(
      mf,
	"</head>"

        "<script type=\"text/javascript\">"

        "$.extend($.fn.validatebox.defaults.rules, {"
           "minLength: {"
              "validator: function(value, param){"
                 "return value.length >= param[0];"
              "},"
              "message: 'Please enter at least {0} characters.'"
           "},"
           "passEquals: {"
              "validator: function(value,param){"
                 "return value == $(param[0]).val();"
              "},"
              "message: 'password do not match.'"
           "}"
        "});"

        "function submitForm()"
        "{"
           "v=$('#index_fm').form('validate');"
           "if(v)"
              "$('#index_fm').submit();"
        "}"

        "jQuery(document).ready(function(){"
           "$.ajaxSetup({ cache: false });"
           "$('#info_box').delay(3000).fadeOut(1000);"
        "});"

        "</script>"

        "<body>"
        "<div style=\"min-width:950px;min-height:650px;margin:10px;text-align:center;\">"
           "<div>"
               "<div style=\"display:inline-block\">"
                  "<img src=\"logo-librewifi.png\" border=\"0\" align=\"center\" width=400px/>"
               "</div>"
           "<div>"
           "<div style=\"display:inline-block;margin-top:50px\">");
   
   if(issubmit!=-1)
   {     
      if(error)
      { 
         mea_memfile_printf(mf, 
//              "<div id=\"info_box\" style=\"margin-bottom:15px;padding-bottom:10px;padding-top:10px;border:1px solid red;color:red;\">");
              "<div id=\"info_box\" style=\"padding:5px;margin-bottom:15px;border:1px solid red;color:red;\">");
         mea_memfile_printf(mf, 
              "<DIV style=\"margin-bottom:10px;\"><B>Data not saved</B></DIV>", error);
         if(error & 0b00000001)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: password confirmation doesn't match password</DIV>", error);
         if(error & 0b00001000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: essid name not set or too long (need 1 to 32 caracters)</DIV>", error);
         if(error & 0b00000010)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: incorrect essid passphrase length (8 to 63 caracters needed)</DIV>", error);
         if(error & 0b00000100)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: essid pass can only have printable characters</DIV>", error);
         if(error & 0b00010000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: incorrect freewifi password length (32 caracters max)</DIV>", error);
         if(error & 0b00100000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: incorrect freewifi user length (32 caracters max)</DIV>", error);
         if((error & 0b01000000) || (error & 0b0010000000000000))
            mea_memfile_printf(mf, 
              "<DIV>ERROR: invalid box ip address</DIV>", error);
         if(error & 0b10000000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: invalid netmask</DIV>", error);
         if(error & 0b0000000100000000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: invalid first dhcp address</DIV>", error);
         if(error & 0b0000001000000000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: invalid last dhcp address</DIV>", error);
         if(error & 0b0000010000000000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: first dhcp address not in box network</DIV>", error);
         if(error & 0b0000100000000000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: last dhcp address not in box network</DIV>", error);
         if(error & 0b0001000000000000)
            mea_memfile_printf(mf, 
              "<DIV>ERROR: box ip address in dhcp range</DIV>", error);
         mea_memfile_printf(mf, 
           "</div>");
      }
      else
      {
         mea_memfile_printf(mf, 
           "<div id=\"info_box\" style=\"padding:5px;margin-bottom:15px;border:1px solid green;color:green;\">");
         mea_memfile_printf(mf, 
           "<DIV><B>New parameters successully saved</B></DIV>");
         mea_memfile_printf(mf, 
           "</div>");
      }
   }
   else
   {
      mea_memfile_printf(mf, 
           "<div id=\"info_box\" style=\"display:none;padding:5px;margin-bottom:15px;border:1px solid black;color:black;\"></DIV>");
   }

   mea_memfile_printf(
      mf,
           "<form id=\"index_fm\" method=post>"
              "<div id=\"p1\" class=\"easyui-panel\" data-options=\"style:{margin:'0 auto'}\" title=\"Admin account\" style=\"width:700px;padding:10px;margin-bottom:25px;\">"
                 "<table width=\"600px\" align=\"center\" cellpadding=\"5\" style=\"padding-bottom: 5px;\">"
                    "<col width=\"35%%\">"
                    "<col width=\"65%%\">"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"pass1\" id=\"pass1_lbl\">password:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" type=\"password\" style=\"height:25px;width:100%%;margin-bottom:0px;\" name=\"pass1\" id=\"pass1\" value=\"%s\" data-options=\"required:true,validType:'minLength[6]'\">"
                       "</td>"
                    "</tr>"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"pass2\" id=\"pass2_lbl\">password confirmation:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" type=\"password\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"pass2\" id=\"pass2\" value=\"%s\" data-options=\"required:true\" validType=\"passEquals['#pass1']\">"
                       "</td>"
                    "</tr>"
                 "</table>"
              "</div>"

              "<div id=\"p2\" class=\"easyui-panel\" data-options=\"style:{margin:'0 auto'}\" title=\"Librewifi access point\" style=\"width:700px;padding:10px;margin-bottom:25px;\">"
                 "<table width=\"600px\" align=\"center\" cellpadding=\"5\" style=\"padding-bottom: 5px;\">"
                    "<col width=\"35%%\">"
                    "<col width=\"65%%\">"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"essid_name\" id=\"essid_name_lbl\">essid:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"essid_name\" id=\"essid_name\" value=\"%s\" data-options=\"required:true,validType:'length[1,32]'\">"
                       "</td>"
                    "</tr>"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"essid_pass\" id=\"essid_pass_lbl\">WPA2 pass phrase:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"essid_pass\" id=\"essid_pass\" value=\"%s\" data-options=\"required:true,validType:'length[8,63]'\">"
                       "</td>"
                    "</tr>"


                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"ipbox1\" id=\"ipbox1_lbl\">box IP adress:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"ipbox1\" id=\"ipbox1\" value=\"%s\" data-options=\"required:true\">"
                       "</td>"
                    "</tr>"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"netmask1\" id=\"netmask1_lbl\">box netmask:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"netmask1\" id=\"netmask1\" value=\"%s\" data-options=\"required:true\">"
                       "</td>"
                    "</tr>"


                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"ipstart1\" id=\"ipstart1_lbl\">dhcp first address:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"ipstart1\" id=\"ipstart1\" value=\"%s\" data-options=\"required:true\">"
                       "</td>"
                    "</tr>"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"ipend1\" id=\"ipend1_lbl\">dhcp last address:</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"ipend1\" id=\"ipend1\" value=\"%s\" data-options=\"required:true\">"
                       "</td>"
                    "</tr>"


                 "</table>"
              "</div>"

              "<div id=\"p3\" class=\"easyui-panel\" data-options=\"style:{margin:'0 auto'}\" title=\"Freewifi connection parameters\" style=\"width:700px;padding:10px;margin-bottom:10px;\">"
                 "<table width=\"600px\" align=\"center\" cellpadding=\"5\" style=\"padding-bottom: 5px;\">"
                    "<col width=\"35%%\">"
                    "<col width=\"65%%\">"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"fwuser\" id=\"fwuser_lbl\">user id</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"fwuser\" id=\"fwuser\" value=\"%s\" data-options=\"required:true\">"
                       "</td>"
                    "</tr>"
                    "<tr>"
                       "<td align=\"right\">"
                          "<label for=\"fwpass\" id=\"fwpass_lbl\">password</label>"
                       "</td>"
                       "<td>"
                          "<input class=\"easyui-textbox\" style=\"height:25px; width:100%%; margin-bottom:0px;\" name=\"fwpass\" id=\"fwpass\" value=\"%s\" data-options=\"required:true\">"
                       "</td>"
                    "</tr>"
                 "</table>"
              "</div>"
              "<input type=hidden name=submited value=1>"
           "</form>"

           "<div style=\"text-align:center; margin:20px 0;padding:10px;\">"
              "<a href=\"javascript:void(0)\" id=\"save\" class=\"easyui-linkbutton\" style=\"margin:5px;\" data-options=\"iconCls:'icon-ok'\" onclick=\"submitForm();\">"
                 "<span style=\"font-size:14px;\">Save</span>"
              "</a>"
              "<a href=\"logout.html\" id=\"logout\" class=\"easyui-linkbutton\" style=\"margin:5px;\">"
                 "<span style=\"font-size:14px;\">Logout</span>"
              "</a>"
           "</div>"
           "</div>"
        "</div>"
        "</body>"
      "</html>",
      pass1,
      pass2,
      essid_name,
      essid_pass,
      ip,
      netmask,
      dhcp_start,
      dhcp_end,
      freewifi_login,
      freewifi_pass
   );

   _httpResponse(conn, mf->data);

page_index_clean_exit:
   if(params)
   {
      mea_cfgfile_clean(params);
      free(params);
      params=NULL;
   }
   
   if(mf)
   {
      mea_memfile_release(mf);
      free(mf);
      mf=NULL;
   }
   return 1;
}


static int page_logout(struct mg_connection *conn, struct session_s *session)
{
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&sessions_rwlock );
   pthread_rwlock_wrlock(&sessions_rwlock);

   struct session_s *e=find_session(&sessions, session->id);
   if(e != NULL)
   {
      mea_queue_remove_current(&sessions);
      free(e);
      e=NULL;
   }

   pthread_rwlock_unlock(&sessions_rwlock);
   pthread_cleanup_pop(0);

   _httpResponseRedirect(conn, "/login.html");

   return 0;
}


static int page_login(struct mg_connection *conn, struct session_s *session)
{
   const char *cl;
   int login_ret=-1;
   int pass_ret=-1;
   char login[20];
   char pass[20];
   char admin_pass[20];
   int error=-1;

   struct mea_memfile_s *mf=NULL;

//   struct cfgfile_params_s *params = mea_cfgfile_params_alloc(user_params_keys_list);
   struct cfgfile_params_s *params = mea_cfgfile_params_from_string_alloc(user_params_keys_list_str);
   if(params==NULL)
   {
      _httpResponse(conn, "Internal error");
      VERBOSE(1) mea_log_printf("%s (%s) : can't alloc user configuration file\n", ERROR_STR, __func__);
      return 1;
   }

   int ret=mea_cfgfile_load(val_user_params_file, params);
   if(ret<0)
   {
      _httpResponse(conn, "Internal error");
      VERBOSE(2) mea_log_printf("%s (%s) : can't load parameters file (%s)\n", ERROR_STR, __func__, val_user_params_file);
      mea_cfgfile_clean(params);
      free(params); 
      return 1;
   }
   strcpy(admin_pass, mea_cfgfile_get_value_by_id(params, ADMIN_PASSWORD_ID));
 
   mea_cfgfile_clean(params);
   free(params);
   params=NULL;
 
   session->logged_in=0; 
   if ((cl = mg_get_header(conn, "Content-Length")) != NULL)
   {
      char *buf;
      int len = atoi(cl);
      if ((buf = malloc(len)) != NULL)
      {
         mg_read(conn, buf, len);

         login_ret=mg_get_var(buf, len, "name", login, sizeof(login));
         pass_ret=mg_get_var(buf, len, "pass", pass, sizeof(pass));

         free(buf);
      }
   }

   if(cl && (pass_ret != -1 || login_ret !=-1))
   {
      if(strcmp(login,"admin")==0 && strcmp(pass,admin_pass)==0)
      {
         session->logged_in=1;
         _httpResponseRedirect(conn, "/index.html");
         return 1;
      }
      error=0;
   }

   mf=mea_memfile_init(mea_memfile_alloc(),AUTOEXTEND,1024,1);
   if(mf==NULL)
   {
      _httpResponse(conn, "Internal error");
      return 1;
   }

   mea_memfile_printf(mf,
      "<html>"
      "<head>"
         "<title>"
            "LibreWifi"
         "</title>");
   mea_memfile_include(mf, "/data/librewifi2/www/include/libs.inc", 0);
   mea_memfile_printf(mf,
      "</head>"
      
      "<script type=\"text/javascript\">"

      "function login()"
      "{"
         "$('#login_fm').submit();"
      "}"

      "jQuery(document).ready(function(){"
         "$.ajaxSetup({ cache: false });"
         "$('#info_box').delay(3000).fadeOut(1000);"
         "$('#login').click(login);"
      "});"      
      "</script>"

      "<body>"
      "<div style=\"min-width:950px;min-height:650px;margin:10px;text-align:center;\">"
         "<div>"
            "<div style=\"display:inline-block\">"
               "<img src=\"logo-librewifi.png\" border=\"0\" align=\"center\" width=400px/>"
            "</div>"
         "<div>"
         "<div style=\"display:inline-block;margin-top:50px\">");

   if(error==0)
   {
      mea_memfile_printf(mf,
            "<div id=\"info_box\" style=\"padding:5px;margin-bottom:15px;border:1px solid red;color:red;\">"
               "<div><B>Can't validate user password. Check it !</B></div>"
            "</div>");
   }

   mea_memfile_printf(mf,
            "<div class=\"easyui-panel\" title=\"Connection\" style=\"width:400px;padding:30px 70px 20px 70px;margin:0 auto\">"
               "<form id=login_fm method=post>"
                  "<div style=\"margin-bottom:10px\">"
                     "<input id=\"name\" name=\"name\" class=\"easyui-textbox\" style=\"width:100%%;height:40px;padding:12px\" data-options=\"prompt:'login',iconCls:'icon-man',iconWidth:38\">"
                  "</div>"
                  "<div style=\"margin-bottom:20px\">"
                     "<input id=\"pass\" name=\"pass\" class=\"easyui-textbox\" type=\"password\" style=\"width:100%%;height:40px;padding:12px\" data-options=\"prompt:'password',iconCls:'icon-lock',iconWidth:38\">"
                  "</div>"
                  "<a href=\"#\" id=\"login\" class=\"easyui-linkbutton\" data-options=\"iconCls:'icon-ok'\" style=\"padding:5px 0px;width:100%%;\">"
                  "<span style=\"font-size:14px;\">Login</span>"
                  "</a>"
               "</form>"
            "</div>"
         "</div>"
      "</div>"
      "</body>"
      "</html>");

   _httpResponse(conn, mf->data);

   if(mf)
   {
      mea_memfile_release(mf);
      free(mf);
      mf=NULL;
   }

   return 1;
}


struct pages_s
{
   int id;
   char *page_name;
   int (*page_f)(struct mg_connection *conn, struct session_s *session);
   
};


struct pages_s _allpages[]={{0,"/index.html",page_index},{1,"/login.html",page_login},{2, "/logout.html", page_logout},{-1,NULL,NULL}};

static int pages(struct mg_connection *conn, struct session_s *session)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   int i=0;
/*
   DEBUG_SECTION {
      mea_log_printf("query_string = %s, is_ssl = %d\n", request_info->query_string, request_info->is_ssl);
      int i=0;
      for(;i<request_info->num_headers;i++)
      {
         mea_log_printf("header[%d] : %s = %s\n", i, request_info->http_headers[i].name, request_info->http_headers[i].value); 
      }
   }
*/
   for(;_allpages[i].page_name;i++)
   {
      if(strcmp(_allpages[i].page_name,(char *)request_info->uri)==0)
         return _allpages[i].page_f(conn,session);
   }

   return 0;
}


static int _begin_request_handler(struct mg_connection *conn)
{
   char mea_sessid[IW_ESSID_MAX_SIZE+1];
   struct session_s *session=NULL;

   const struct mg_request_info *request_info = mg_get_request_info(conn);

   if(request_info->is_ssl != 1)
   {
      char *host;
      char *dest;

      const char *_host=mg_get_header(conn, "Host");
      host=(char *)malloc(strlen(_host)+1);
      sscanf(_host,"%[^:]:",host);
      dest=(char *)malloc(strlen(host)+26);
      sprintf(dest,"https://%s:%d/index.html",host,7443);
      _httpResponseRedirect(conn, dest);
      free(dest);
      free(host);
      return 1;
   }

   const char *cookie=mg_get_header(conn, "Cookie");

   // recherche de l'état de la session
   int ret=mg_get_cookie(cookie, "MEASESSID", mea_sessid, sizeof(mea_sessid));
   if(ret>0)
   {
      struct session_s *e;

      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&sessions_rwlock );
      pthread_rwlock_wrlock(&sessions_rwlock);
      if(mea_queue_first(&sessions)==0)
      {
         while(mea_queue_current(&sessions, (void **)&e)==0)
         {
            if(strcmp(mea_sessid, e->id) == 0)
            {
               time_t now = time(NULL);

               if(difftime(now, e->last)>(10.0*60.0)) // 10 mn pour une session
               {
                  mea_queue_remove_current(&sessions);
                  free(e);
                  e=NULL;
               }
               else
               {
                  e->last=now;
                  session=e;
               }
               break;
            }
            mea_queue_next(&sessions);
         }
      }
      pthread_rwlock_unlock(&sessions_rwlock);
      pthread_cleanup_pop(0);
   }

   if(session==NULL) // aucune session valide
   {
      char *id_session = NULL;

      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&sessions_rwlock );
      pthread_rwlock_rdlock(&sessions_rwlock);
      do
      {
         id_session = new_id_session_alloc(32);
      }
      while(find_session(&sessions, id_session)!=NULL);

      session = (struct session_s *)malloc(sizeof(struct session_s));
      if(session==NULL)
      {
         // traitement d'erreur à faire
         goto next;
      }

      strcpy(session->id, id_session);

      session->first=time(NULL);
      session->last=session->first;
      session->logged_in=0;
      session->counter=0;
      mea_queue_in_elem(&sessions,(void *)session);

      free(id_session);

next:
      pthread_rwlock_unlock(&sessions_rwlock);
      pthread_cleanup_pop(0);

      _httpResponseSetSessionAndRedirect(conn, session->id, "/login.html");

      return 1;
   }
   else
   {
      (session->counter)++;

      return pages(conn, session);
   }

   return 1;
}


void donothing_handler(int signo)
{
   DEBUG_SECTION fprintf(stderr,"J'ai recu le signal %d alors qu'il devrait être masqué !!!\n", signo);
}


static void signals_handler(int signo)
{
   if(signo==SIGTERM)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : stop gui server\n", INFO_STR, __func__);

      if(g_mongooseContext)
      {
         mg_stop(g_mongooseContext);
         g_mongooseContext=NULL; 
      }

      if(val_document_root)
      {
         free(val_document_root);
         val_document_root=NULL;
      }

      if(val_listening_ports)
      {
         free(val_listening_ports);
         val_listening_ports=NULL;
      }

      if(val_num_threads)
      {
         free(val_num_threads);
         val_num_threads=NULL;
      }

      if(val_sslpem)
      {
         free(val_sslpem);
         val_sslpem=NULL;
      }

      VERBOSE(5) mea_log_printf("%s (%s) : gui server stopped\n", INFO_STR, __func__);

      exit(0);
   }
   else if(signo==SIGUSR1)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : change configuration will not provocate reboot\n", DEBUG_STR, __func__);
      reboot_flag=0;
   }
   else if(signo==SIGUSR2)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : change configuration will provocate reboot\n", DEBUG_STR, __func__);
      reboot_flag=1;
   }
}


int stop_httpServer()
{
   int status;

   if(_guiServer_pid != -1)
   {
      int ret=kill(_guiServer_pid, SIGTERM);
      waitpid(_guiServer_pid, &status, 0);
      _guiServer_pid=-1;
   }


   return 0;
}


int set_reboot_flag(int flag)
{
   if(_guiServer_pid != -1)
   {
      if(flag==1)
         kill(_guiServer_pid, SIGUSR2);
      else
         kill(_guiServer_pid, SIGUSR1);
      return 0;
   }
   return -1;
}


mea_error_t start_httpServer(char *home, char *bind_addr, uint16_t port_http, uint16_t port_https, char *sslpem, char *user_params_file)
{
   int pid;

   if(_guiServer_pid != -1)
   {
      return -1;
   }

   if ((pid = fork()) < 0)
      return -1;

   if (pid == 0) // dans le fils, on lance la commande
   {
      VERBOSE(5) mea_log_printf("%s (%s) : start gui server\n", INFO_STR, __func__);

      if(port_http==0)
         port_http=7080;
      if(port_https==0)
         port_https=7443;

      mea_queue_init(&sessions);

      val_listening_ports=(char *)malloc(80);

      if(bind_addr)
      { 
         if(port_https)
            sprintf(val_listening_ports,"%s:%d,%s:%ds",bind_addr,port_http,bind_addr,port_https);
         else
            sprintf(val_listening_ports,"%s:%d",bind_addr,port_http);
      }
      else
      {
         if(port_https)
            sprintf(val_listening_ports,"%d,%ds",port_http,port_https);
         else
            sprintf(val_listening_ports,"%d",port_http);
      }

      val_document_root=malloc(strlen(home)+1);
      strcpy(val_document_root,home);
     
      val_sslpem=malloc(strlen(sslpem)+1);
      strcpy(val_sslpem,sslpem);
    
      val_user_params_file=malloc(strlen(user_params_file)+1);
      strcpy(val_user_params_file, user_params_file);

      options[ 0]=str_document_root;
      options[ 1]=(const char *)val_document_root;
      options[ 2]=str_listening_ports;
      options[ 3]=(const char *)val_listening_ports;
      options[ 4]=str_index_files;
      options[ 5]="index.html";
      options[ 6]=str_num_threads;
      options[ 7]="5";
      options[ 8]="ssl_certificate";
      options[ 9]=(const char *)val_sslpem;
      options[10]="enable_directory_listing";
      options[11]="no";
      options[12]="enable_keep_alive";
      options[13]="no";
      options[14]=NULL;
  
      struct mg_callbacks callbacks;
   
      memset(&callbacks, 0, sizeof(struct mg_callbacks));
      callbacks.begin_request = _begin_request_handler;
//      callbacks.open_file = open_file_handler;
  
      signal(SIGTERM, signals_handler);
      signal(SIGUSR1, signals_handler);
      signal(SIGUSR2, signals_handler);

      sigset_t ss;
      sigemptyset (&ss);
      sigaddset(&ss, SIGHUP);
      sigaddset(&ss, SIGINT);
      sigaddset(&ss, SIGQUIT);
      int ret=sigprocmask(SIG_BLOCK, &ss, NULL);

      g_mongooseContext = mg_start(&callbacks, NULL, options);
      if (g_mongooseContext == NULL)
      {
         VERBOSE(5) mea_log_printf("%s (%s) : gui server not started\n", INFO_STR, __func__);
         int ppid=getppid();
         if(ppid!=1) // papa est il encore vivant ?
         {
            kill(ppid, SIGTERM); // oui, c'est lui qui doit me stopper quand il va s'arrêter. Je lui dit de la faire ...
         }
         else
            signals_handler(SIGTERM); // non, je suis grand, je décide de m'arrêter tout seul ...

         sleep(30);

         exit(1);
      }

      VERBOSE(5) mea_log_printf("%s (%s) : gui server started\n", INFO_STR, __func__);

      for(;;)
      {
         sleep(1);
         if(getppid()==1) // si j'ai perdu mon pere, je m'arrête proprement aussi ...
         {
            VERBOSE(5) mea_log_printf("%s (%s) : form gui server : my father is dead, I go down too.\n", INFO_STR, __func__);
            signals_handler(SIGTERM);
         }
      }
   }
   else
   {
      _guiServer_pid=pid;
      return 0;
   }
}


int stop_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct guiServerData_s *guiServerData = (struct guiServerData_s *)data;

   stop_httpServer();

   return 0;
}


int start_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct guiServerData_s *guiServerData = (struct guiServerData_s *)data;

   _guiServer_id=my_id;

   int ret=start_httpServer(guiServerData->home, guiServerData->addr, guiServerData->http_port, guiServerData->https_port, guiServerData->sslpem, guiServerData->user_params_file);

   return 0;
}


int restart_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_guiServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_guiServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}
