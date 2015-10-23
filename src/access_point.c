#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "mea_memfile.h"
#include "mea_cfgfile_utils.h"

#include "access_point.h"


int restart_hostapd_service()
{
   system("/usr/sbin/service hostapd restart");
}


int reboot()
{
   sleep(5);

   return system("/sbin/init 6");
}


int create_hostapd_cfg(char *template_file, char *dest_file, char *iface, char *essid, char *passwd)
{
   return mea_create_file_from_template(template_file, dest_file, "iface", iface, "essid", essid, "passwd", passwd, NULL);
}


int create_interfaces_cfg(char *template_file, char *dest_file, char *ip_and_netmask)
{
   char ip[32], netmask[32];
   int n;
 
   sscanf(ip_and_netmask,"%32[^,],%32[^/n]%n", ip, netmask, &n);
   mea_strtrim2(ip);
   mea_strtrim2(netmask);
   
   return mea_create_file_from_template(template_file, dest_file, "boxip", ip, "netmask", netmask, NULL);
}


int create_udhcpd_cfg(char *template_file, char *dest_file, char *iface, char *ip_and_netmask, char *dhcp_range)
{
   char ip[32], netmask[32];
   char dhcp_start[32], dhcp_end[32];
   int n;

   sscanf(ip_and_netmask, "%32[^,],%32[^/n]%n", ip, netmask, &n);
   sscanf(dhcp_range, "%32[^,],%32[^/n]%n", dhcp_start, dhcp_end, &n);

   mea_strtrim2(ip);
   mea_strtrim2(netmask);
   mea_strtrim2(dhcp_start);
   mea_strtrim2(dhcp_end);

   return mea_create_file_from_template(template_file, dest_file, "iface", iface, "boxip", ip, "netmask", netmask, "start", dhcp_start, "end", dhcp_end, NULL);
}

/*
int create_hostapd_cfg_old(char *template_file, char *dest_file, char *iface, char *essid, char *passwd)
{
   FILE *fd_in=NULL, *fd_out=NULL;
   struct memfile_s *mf=NULL;
   char line[255];
   int ret_code=-1;

   fd_in=fopen(template_file, "r");
   if(!fd_in)
   {
      fprintf(stderr,"can't open template file %s : ", template_file);
      perror("");
      goto create_hostapcfg_clean_exit;
   }

   mf=memfile_init(memfile_alloc(), AUTOEXTEND, 1024, 1);

   int lnum=0;
   while(!feof(fd_in))
   {
      if(fgets(line,sizeof(line)-1,fd_in) != NULL)
      {
         lnum++;
         char var[80]="";
         int i=0;
         for(;line[i];i++)
         {
            if(line[i]=='[' && line[i+1]=='[')
            {


               int j=0;
               int flag=-1;
               for(;line[i];i++)
               {
                  if(line[i]==']' && line[i+1]==']')
                  {
                     i+=1;
                     var[j]=0;
                     flag=0;
                     break;
                  }
                  else
                  {
                     var[j++]=line[i];
                  }
               }
               if(flag==0)
               {
                  if(strcmp(var,"essid")==0)
                  {
                       memfile_printf(mf, "%s", essid);
                  }
                  else if(strcmp(var, "iface")==0)
                  {
                     memfile_printf(mf, "%s", iface);
                  }
                  else if(strcmp(var, "passwd")==0)
                  {
                     memfile_printf(mf, "%s", passwd);
                  }
                  else
                  {
                     fprintf(stderr,"unknow substitution tag (%s) at line %d\n",var,lnum);
                     goto create_hostapcfg_clean_exit;
                  }
               }
               else
               {
                  fprintf(stderr,"Template error line %d\n", lnum);
                  goto create_hostapcfg_clean_exit;
               }
               var[0]=0;
            }
            else
            {
               memfile_printf(mf, "%c", line[i]);
            }
         }
      }
   }
  
   fd_out=fopen(dest_file, "w");
   if(!fd_out)
   {
      fprintf(stderr,"can't create/open %s : ", dest_file);
      perror("");
      goto create_hostapcfg_clean_exit;
   }

   int i=0;
   for(;mf->data[i];i++) 
   {
      putc(mf->data[i], stderr);
      putc(mf->data[i], fd_out);
   }

   fclose(fd_out);

   ret_code=0;

create_hostapcfg_clean_exit:
   if(fd_in!=NULL)
      fclose(fd_in);

   if(mf)
   {
      memfile_release(mf);
      free(mf);
      mf=NULL;
   }

   return ret_code;
}
*/
