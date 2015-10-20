#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "mea_verbose.h"
#include "mea_utils.h"
#include "minidisplay.h"
#include "wifi.h"
#include "scanner.h"

#include "debug.h"

pthread_t *_scannerServer_thread=NULL;

char scanner_display_page=-1;

#define DISPLAY_PAGE scanner_display_page

char scanner_flag=0;

void scanner_on()  { scanner_flag = 1; };
void scanner_off() { scanner_flag = 0; };

static void _wifi_scanner(char *iface, char *essid_searched, uint16_t max, struct mini_display_s *display)
{
   int skfd;  // generic raw socket desc
   char *dev; // device name
   char *cmd; // command

   // Create a channel to the NET kernel
   if((skfd = iw_sockets_open()) < 0)
   {
      perror("socket");
      return;
   }

   struct ap_data_s *ap_data=ap_data_get_alloc(skfd, iface, essid_searched, 0);
   if(ap_data)
   {
      int i=0;
      uint16_t nb=0;
      char shorted_essid[13];

      for(;ap_data[i].index!=-1 && nb<max;i++)
      {
         if(essid_searched==NULL || essid_searched[0]==0 || strcmp(ap_data[i].essid, essid_searched)==0)
         {
            if(display)
            {
               strncpy(shorted_essid,ap_data[i].essid,12);
               mini_display_xy_printf(display,DISPLAY_PAGE,0,nb,1,"%-12s %3.0f/%3f", shorted_essid, ap_data[i].quality*100.0, ap_data[i].lvl);
            }
            DEBUG_SECTION mea_log_printf("%s (%s) : ESSID - %s (%.2f%%,%.2fdb)\n", DEBUG_STR, __func__, ap_data[i].essid, ap_data[i].quality*100.0, ap_data[i].lvl);

            nb++;
         }
      }
      for(;nb<max;nb++)
      {
         if(display)
            mini_display_xy_printf(display,DISPLAY_PAGE,0,nb,1,"");
      }
   
      FREE(ap_data);
   }

  // Close the socket
   iw_sockets_close(skfd);
}


struct _scanner_thread_data_s
{
   int myID;
   char essid[40];
   char iface[40];
   struct mini_display_s *display;
};


void *_scanner_thread(void *args)
{
   char iface[40]="wlan0";
   char essid[40]="";
   struct mini_display_s *display=NULL;
   int myID;

   struct _scanner_thread_data_s *_args=(struct _scanner_thread_data_s *)args;

   if(_args!=NULL)
   {
      strncpy(iface,_args->iface,sizeof(iface)-1);
      strncpy(essid,_args->essid,sizeof(essid)-1); 
      display=_args->display;
      myID=_args->myID;
      FREE(args);
   }
   else
      return NULL;

   if(display)
   {
      mini_display_clear_screen(display,DISPLAY_PAGE);
      mini_display_xy_printf(display, DISPLAY_PAGE, 4, 1, 1, "WIFI SCANNER");
   }

   mea_timer_t scan_timer;
   mea_init_timer(&scan_timer, 1, 1);
   mea_start_timer(&scan_timer);

   DEBUG_SECTION mea_log_printf("%s (%s) : scanner server is running\n", DEBUG_STR, __func__);

   while(1)
   {
      if(mea_test_timer(&scan_timer)==0)
      {
         if(scanner_flag)
            _wifi_scanner(iface, essid, 4, display);
         process_heartbeat(myID);
      }
      usleep(100*1000); // 250 ms
   }
}


void scanner_thread_set_display_page(int page)
{
   scanner_display_page=(char)page;
}


pthread_t *scanner_thread_start(int myID, char *iface, char *essid, struct mini_display_s *display)
{
   pthread_t *thread=NULL;

   struct _scanner_thread_data_s *args=(struct _scanner_thread_data_s *)malloc(sizeof(struct _scanner_thread_data_s));
   if(args==NULL)
      return NULL;

   strncpy(args->iface,iface,sizeof(args->iface)-1);
   strncpy(args->essid,essid,sizeof(args->iface)-1);
   args->display=display;
   args->myID=myID;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      FREE(args);
      args=NULL;
      return NULL;
   }

   if(pthread_create(thread, NULL, (void *)_scanner_thread, (void *)args))
   {
      FREE(args);
      args=NULL;
      return NULL;
   }

   pthread_detach(*thread);

   return thread;
}


int stop_scannerServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   VERBOSE(5) mea_log_printf("%s (%s) : stop scanner server\n", INFO_STR, __func__);

   if(_scannerServer_thread)
   {
      pthread_cancel(*_scannerServer_thread);

      usleep(500*1000);

      FREE(_scannerServer_thread);
      _scannerServer_thread=NULL;
   }

   VERBOSE(5) mea_log_printf("%s (%s) : scanner server stopped\n", INFO_STR, __func__);

   return 0;
}


int start_scannerServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct scannerServerData_s *scannerServerData = (struct scannerServerData_s *)data;

   char *iface=scannerServerData->iface;
   char *essid=scannerServerData->essid;
   struct mini_display_s *display=scannerServerData->display;

   VERBOSE(5) mea_log_printf("%s (%s) : start scanner server\n", INFO_STR, __func__);

   _scannerServer_thread = scanner_thread_start(my_id, iface, essid, display);
   if(_scannerServer_thread)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : scanner server started\n", INFO_STR, __func__);
      return 0;
   }
   else
   {
      VERBOSE(5) mea_log_printf("%s (%s) : scanner server not started\n", INFO_STR, __func__);
      return -1;
   }
}


int restart_scannerServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;

   ret=stop_scannerServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_scannerServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}

