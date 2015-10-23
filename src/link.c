#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include <curl/curl.h>

#include "link.h"

#include "mea_ip_utils.h"
#include "mea_verbose.h"
#include "mea_utils.h"
#include "mea_string_utils.h"

#include "minidisplay.h"
#include "auth.h"

#include "debug.h"

pthread_t *_linkServer_thread=NULL;

char link_display_page=-1;

#define DISPLAY_PAGE link_display_page

#define IP_LINE 0
#define PING_LINE 2
#define AUTH_LINE 1

char *ping_hosts_to_check[]={ "www.ibm.com", "www.free.fr", "www.apple.com", "www.microsoft.com", NULL };
int ping_hosts_to_check_rr=0;

char *auth_url_to_check[]={ "http://www.ibm.com",
                            "http://www.apple.com",
                            "http://impots.gouv.fr",
                            "http://www.oracle.com",
                            "http://www.raspberry.org",
                            "http://www.microsoft.com",
                            NULL };
int auth_url_to_check_rr=0;

int set_iptables_nat(char *iface)
{
// iptables -t nat -A POSTROUTING -o $IFACE -j MASQUERADE >> $LOGFILE 2>&1

   /* create the pipe */
   int pfd[2];
   if (pipe(pfd) == -1)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : pipe failed\n",ERROR_STR,__func__);
      return -1;
   }

   /* create the child */
   int pid;
   if ((pid = fork()) < 0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : fork failed\n",ERROR_STR,__func__);
      return -1;
   }

   if (pid == 0) // dans le fils, on lance la commande
   {
      /* child */
      close(pfd[0]);   /* close the unused read side */
      dup2(pfd[1], 1); /* connect the write side with stdout */
      dup2(pfd[1], 2); /* connect the write side with stdout */
      close(pfd[1]);   /* close the write side */

      execl("/sbin/iptables", "/sbin/iptables", "-t", "nat", "-A", "POSTROUTING", "-o", iface, "-j", "MASQUERADE", NULL);

      DEBUG_SECTION mea_log_printf("%s (%s) : excel failed\n", ERROR_STR, __func__);
      return -1;
   }
   else
   {
      FILE *fpfd;
      int status;
      char line[255];

      close(pfd[1]);   /* close the unused write side */

      fpfd=fdopen(pfd[0],"r");
      while(!feof(fpfd))
      {
         char *ptr=fgets(line,254,fpfd);
         DEBUG_SECTION {
            if(ptr)
               DEBUG_SECTION mea_log_printf("%s (%s) : %s\n", INFO_STR, __func__, mea_strrtrim(line));
         }
      }
      waitpid(pid, &status, 0);
      fclose(fpfd);

      return 0;
   }
}

#define IFUPCMND "/sbin/ifup"
#define IFDOWNCMND "/sbin/ifdown"

int ifupdown(int myID, char *iface, int updown)
{
   /* create the pipe */
   int pfd[2];
   if (pipe(pfd) == -1)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : pipe failed\n", ERROR_STR, __func__);
      return -1;
   }

   /* create the child */
   int pid;
   if ((pid = fork()) < 0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : fork failed\n", ERROR_STR, __func__);
      return -1;
   }

   if (pid == 0) // dans le fils, on lance la commande
   {
      /* child */
      close(pfd[0]);   /* close the unused read side */
      dup2(pfd[1], 1); /* connect the write side with stdout */
      dup2(pfd[1], 2); /* connect the write side with stdout */
      close(pfd[1]);   /* close the write side */

      if(updown==1) // up
      {
         execl(IFUPCMND, IFUPCMND, iface, "--verbose", NULL);
      }
      else
      {
         execl(IFDOWNCMND, IFDOWNCMND, iface, "--verbose", NULL);
      }
      
      VERBOSE(2) mea_log_printf("%s (%s) : excel failed\n", ERROR_STR, __func__);
      
      return -1;
   }
   else
   {
      FILE *fpfd;
      int status;

      char line[255];

      close(pfd[1]);   /* close the unused write side */

      fpfd=fdopen(pfd[0],"r");
      while(!feof(fpfd))
      {
         char *ptr=fgets(line,254,fpfd);
         if(ptr)
         {
            DEBUG_SECTION mea_log_printf("%s (%s) : %s\n", DEBUG_STR, __func__, mea_strrtrim(line));
            process_heartbeat(myID);
         }
      }
      waitpid(pid, &status, 0);
      fclose(fpfd); /* close the read side */

      return WEXITSTATUS(status);
   }

   return -1;
}

struct _link_thread_data_s
{
   int myID;
   char essid[40];
   char iface[40];
   char free_id[12];
   char free_passwd[20];
   struct mini_display_s *display;
};


void *_link_thread(void *args)
{
   int myID=-1;
   char iface[40]="";
   char essid[40]="";
   char free_id[12]="";
   char free_passwd[20]="";

   struct mini_display_s *display=NULL;
   int nb_reconnection=0;

   struct _link_thread_data_s *_args=(struct _link_thread_data_s *)args;

   if(_args!=NULL)
   {
      myID=_args->myID;
      strncpy(iface,_args->iface,sizeof(iface)-1);
      strncpy(essid,_args->essid,sizeof(essid)-1);
      strncpy(free_id,_args->free_id,sizeof(free_id)-1);
      strncpy(free_passwd,_args->free_passwd,sizeof(free_passwd)-1);
      display=_args->display;
      FREE(args);
   }

   int ret=0;
   int exit_code=0;


   mea_timer_t auth_timer;
   mea_timer_t ping_timer;
   mea_timer_t auth_ctrl_timer;
   mea_timer_t hb_timer;

   mea_init_timer(&auth_timer, 60*30, 1); // authentification toutes les 30 minutes
   mea_init_timer(&ping_timer, 15, 1); // ping  toutes les 15 secondes
   mea_init_timer(&auth_ctrl_timer, 60, 1);
   mea_init_timer(&hb_timer, 5, 1);

   mea_start_timer(&ping_timer);
   mea_start_timer(&hb_timer);
   
   curl_global_init(CURL_GLOBAL_DEFAULT);

   DEBUG_SECTION mea_log_printf("%s (%s) : link server is running\n", DEBUG_STR, __func__);

   for(;;) // boucle principale
   {
      int ping_error_cntr=0;
      int iptables_flag=0;
      int auth_flag=0;
      int ip_flag;
      char addr[16], addr_last[16];

      if(mea_test_timer(&hb_timer))
      {
         process_heartbeat(myID);
      }

      ip_flag=mea_waitipaddr(iface, 5, addr);
      if(ip_flag==0)
      {
         system("/sbin/iptables -t nat -F");
         iptables_flag=set_iptables_nat(iface);
         DEBUG_SECTION mea_log_printf("%s (%s) : iptables return = %d\n", DEBUG_STR, __func__, iptables_flag);

         if(display)
         {
            mini_display_clear_screen(display, DISPLAY_PAGE);
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, AUTH_LINE, 1, "AUTHENTICATION:  __");
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, PING_LINE, 1, "PING:            __");
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, IP_LINE,   1, "IP: %15s", addr);
            strcpy(addr_last,addr);
         }

         int auth_retry=0;
         do
         {
            if (mea_test_timer(&hb_timer))
            {
               process_heartbeat(myID);
            }
               
            if(auth_flag==0 || mea_test_timer(&auth_timer)==0)
            {
               mea_stop_timer(&auth_ctrl_timer);
               int authret=freewifi_auth(free_id, free_passwd);
               DEBUG_SECTION mea_log_printf("%s (%s) : freewifi_auth return = %d\n", DEBUG_STR, __func__, authret);
               if(authret==0)
               {
                
                  int i=0; 
                  for(;i<3;i++)
                  {
                     if (mea_test_timer(&hb_timer))
                     {
                        process_heartbeat(myID);
                     }
                     if(display)
                        mini_display_xy_printf(display, DISPLAY_PAGE, 17, AUTH_LINE, 0, "%02d", i);
                     if(auth_url_to_check[auth_url_to_check_rr]==NULL)
                        auth_url_to_check_rr=0;
                     authret=freewifi_auth_validation(auth_url_to_check[auth_url_to_check_rr]);
                     auth_url_to_check_rr++;
                     if(authret==0)
                        break;
                  }
               }
               if(authret==0)
               {
                  if(display)
                     mini_display_xy_printf(display, DISPLAY_PAGE, 17, AUTH_LINE, 0, "OK");
                  mea_start_timer(&auth_timer);
                  mea_start_timer(&auth_ctrl_timer);
                  auth_retry=0;
                  auth_flag=1;
               }
               else
               {
                  if(display)
                     mini_display_xy_printf(display, DISPLAY_PAGE, 17, AUTH_LINE, 0, "KO");
                  auth_retry++;
               }
            }


            if(mea_test_timer(&auth_ctrl_timer)==0)
            {
               int auth_ctrl_ret=0;
               int i=0;
               if(display)
                  mini_display_xy_printf(display, DISPLAY_PAGE, 0, AUTH_LINE, 1, "AUTHENTICATION:  --");
               for(;i<3;i++)
               {
                  if (mea_test_timer(&hb_timer)==0)
                  {
                     process_heartbeat(myID);
                  }
                  if(display)
                     mini_display_xy_printf(display, DISPLAY_PAGE, 17, AUTH_LINE, 0, "%02d", i);
                  if(auth_url_to_check[auth_url_to_check_rr]==NULL)
                     auth_url_to_check_rr=0;
                  auth_ctrl_ret=freewifi_auth_validation(auth_url_to_check[auth_url_to_check_rr]);
                  auth_url_to_check_rr++;
                  if(auth_ctrl_ret==0)
                     break;
               }
               if(display)
                  mini_display_gotoxy(display, DISPLAY_PAGE, 17, AUTH_LINE);
               if(auth_ctrl_ret==0)
               {
                  if(display)
                     mini_display_print(display, DISPLAY_PAGE, "OK");
                  auth_flag=1;
               }
               else
               {
                  if(display)
                     mini_display_print(display, DISPLAY_PAGE, "KO");
                  auth_flag=0;
               }
            }

            if(mea_test_timer(&ping_timer)==0)
            {
               int i=0;
               int ping_ok=-1;

               if(strcmp(addr,addr_last)!=0)
               {
                  if(display)
                     mini_display_xy_printf(display, DISPLAY_PAGE, 0, IP_LINE, 1, "IP: %15s", addr);
               }

               
               for(i=0;i<3;i++)
               {
                  if (mea_test_timer(&hb_timer)==0)
                  {
                      process_heartbeat(myID);
                  }


                  if(ping_hosts_to_check[ping_hosts_to_check_rr]==NULL)
                     ping_hosts_to_check_rr=0;

                  if(display)
                     mini_display_xy_printf(display, DISPLAY_PAGE, 0, PING_LINE, 1, "PING:            %02d",i);
                  if (mea_ping(ping_hosts_to_check[ping_hosts_to_check_rr])<0)
                  {
                     DEBUG_SECTION mea_log_printf("%s (%s) : Ping (%s) is not OK.\n", DEBUG_STR, __func__, ping_hosts_to_check[ping_hosts_to_check_rr]);
                     if(display)
                        mini_display_xy_printf(display, DISPLAY_PAGE, 17, PING_LINE, 0, "KO");
                     ping_hosts_to_check_rr++;
                  }
                  else
                  {
                     DEBUG_SECTION mea_log_printf("%s (%s) : Ping (%s) is OK.\n", DEBUG_STR, __func__, ping_hosts_to_check[ping_hosts_to_check_rr]);
                     if(display)
                        mini_display_xy_printf(display, DISPLAY_PAGE, 17, PING_LINE, 0, "OK");
                     ping_ok=0;
                     ping_hosts_to_check_rr++;
                     break;
                  }
               }
               if(ping_ok==0)
                  ping_error_cntr=0;
               else
                  ping_error_cntr++;
               if(ping_error_cntr>2)
                  break;
            }

            usleep(100*1000); // 100ms
         }
         while(mea_waitipaddr(iface,5,addr)==0 && auth_retry<3);
      }

      if(display)
      {
         mini_display_clear_screen(display,DISPLAY_PAGE);
         mini_display_change_page(display,DISPLAY_PAGE);
      }

      process_update_indicator(myID, "NB_RECONNECTION", ++nb_reconnection);

      VERBOSE(5) mea_log_printf("%s (%s) : shutdown %s ...\n",INFO_STR,__func__, iface);
      if(display)
         mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 0, "ifdown %s ...", iface);
      int ifdownret=ifupdown(myID, iface, 0);
      DEBUG_SECTION mea_log_printf("%s (%s) : ifdown %s return = %d\n", DEBUG_STR, __func__, iface, ifdownret);
      process_heartbeat(myID);

      VERBOSE(5) mea_log_printf("%s (%s) : wait for %s ...\n", INFO_STR, __func__, essid);
      if(display)
         mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 1, "wait %s ...", essid);
      ret=waitessid(iface, essid, 0.0, 15);
      if(ret<0)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : waitessid can't get info !\n", ERROR_STR, __func__);
         if(display)
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 1, "ERROR");
         goto next;
      }
      if(ret==0)
      {
         VERBOSE(5) mea_log_printf("%s (%s) : no %s network in range !\n", INFO_STR, __func__, essid);
         if(display)
         {
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 1, "no %s", essid);
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, 1, 1, "in range", essid);
         }
         goto next;
      }
      if(display)
      {
         mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 1, "wait %s: %dx", essid, ret);
         mini_display_xy_printf(display, DISPLAY_PAGE, 0, 1, 1, "ifup %s ...", iface);
      }
      fprintf(stderr,"ICI5 (%d)\n", time(NULL));
      process_heartbeat(myID);

      ifdownret=ifupdown(myID, iface, 1);
      if(ifdownret==0)
      {
         if(display)
         {
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, 1, 1, "ifup %s: DONE", iface);
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, 2, 1, "wait ip addr ...");
         }
      }
      else
         if(display)
            mini_display_xy_printf(display, DISPLAY_PAGE, 0, 1, 1, "ifup %s: KO", iface);
next:
      process_heartbeat(myID);
   }

_link_thread_clean_exit:
   curl_global_cleanup();
   VERBOSE(5) mea_log_printf("%s (%s) : link server is down\n", INFO_STR, __func__);
   exit(exit_code);
}


void link_thread_set_display_page(int page)
{
   link_display_page=(char)page;
}


pthread_t *link_thread_start(int myID, char *iface, char *essid, char *free_id, char *free_passwd, struct mini_display_s *display)
{
   pthread_t *thread=NULL;

   struct _link_thread_data_s *args=(struct _link_thread_data_s *)malloc(sizeof(struct _link_thread_data_s));
   if(args==NULL)
      return NULL;

   args->myID=myID;
   strncpy(args->iface,iface,sizeof(args->iface)-1);
   strncpy(args->essid,essid,sizeof(args->essid)-1);
   strncpy(args->free_id,free_id,sizeof(args->free_id)-1);
   strncpy(args->free_passwd,free_passwd,sizeof(args->free_passwd)-1);
   args->display=display;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      FREE(args);
      args=NULL;
      return NULL;
   }

   if(pthread_create(thread, NULL, (void *)_link_thread, (void *)args))
   {
      FREE(args);
      args=NULL;
      return NULL;
   }

   pthread_detach(*thread);

   return thread;
}


int stop_linkServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   VERBOSE(5) mea_log_printf("%s (%s) : stop link server\n", INFO_STR, __func__);

   if(_linkServer_thread)
   {
      pthread_cancel(*_linkServer_thread);

      usleep(100*1000);

      FREE(_linkServer_thread);
      _linkServer_thread=NULL;
   }

   VERBOSE(5) mea_log_printf("%s (%s) : link server stopped\n", INFO_STR, __func__);

   return 0;
}


int start_linkServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct linkServerData_s *linkServerData = (struct linkServerData_s *)data;
  
   char *iface=linkServerData->iface;
   char *essid=linkServerData->essid;
   char *free_id=linkServerData->free_id;
   char *free_passwd=linkServerData->free_passwd;

   struct mini_display_s *display=linkServerData->display;

   VERBOSE(5) mea_log_printf("%s (%s) : start link server\n", INFO_STR, __func__);

   _linkServer_thread = link_thread_start(my_id, iface, essid, free_id, free_passwd, display);
   if(_linkServer_thread)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : link server started\n", INFO_STR, __func__);
      return 0;
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s (%s) : link server not started\n", ERROR_STR, __func__);
      return -1;
   }
}


int restart_linkServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;

   ret=stop_linkServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_linkServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}
