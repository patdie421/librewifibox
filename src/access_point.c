#include <stdio.h>
#include <stdlib.h>
#include "memfile.h"

#include "access_point.h"

int restart_hostapd_service()
{
   int ret=0;

   ret=system("/usr/sbin/service hostapd stop");
   if(ret!=0)
      return ret;
   sleep(1);
   return system("/usr/sbin/service hostapd start");
}


int reboot()
{
   sleep(5);

   return system("/sbin/init 6");
}


int create_hostapd_cfg(char *template_file, char *dest_file, char *iface, char *essid, char *passwd)
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
      goto populate_hostapcfg_clean_exit;
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
               i+=2;
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
                     goto populate_hostapcfg_clean_exit;
                  }
               }
               else
               {
                  fprintf(stderr,"Template error line %d\n", lnum);
                  goto populate_hostapcfg_clean_exit;
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
      goto populate_hostapcfg_clean_exit;
   }

   int i=0;
   for(;mf->data[i];i++) 
   {
      putc(mf->data[i], stderr);
      putc(mf->data[i], fd_out);
   }

   fclose(fd_out);

   ret_code=0;

populate_hostapcfg_clean_exit:
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
