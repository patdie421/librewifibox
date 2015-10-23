#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "stats.h"

#include "mea_verbose.h"
#include "mea_utils.h"
#include "minidisplay.h"
#include "mea_ip_utils.h"

#include "debug.h"

pthread_t *_statsServer_thread=NULL;

char stats_display_page=-1;

#define DISPLAY_PAGE stats_display_page

int iface_stats(char *iface, float *rx, float *tx)
{
   FILE *fd;
   int ret=-1;
   char file[80];

   float rxbps, txbps;

   long rx_value_now=0L, tx_value_now=0L;
   static long rx_value_last=0L, tx_value_last=0L;

   double tx_time_now, rx_time_now;
   static double tx_time_last, rx_time_last;

   *rx=-1.0;
   *tx=-1.0;

   sprintf(file,"/sys/class/net/%s/statistics/tx_bytes", iface);
   fd=fopen(file,"r");
   if(fd==NULL)
      return -1;
   fscanf(fd,"%ld", &tx_value_now);
   tx_time_now=mea_now();
   fclose(fd);

   sprintf(file,"/sys/class/net/%s/statistics/rx_bytes", iface);
   fd=fopen(file,"r");
   if(fd==NULL)
      return -1;
   fscanf(fd,"%ld", &rx_value_now);
   rx_time_now=mea_now();
   fclose(fd);

   if(rx_value_last!=0)
   {
      long d=rx_value_now - rx_value_last;
      double t = rx_time_now - rx_time_last;
 
      rxbps=(float)(d/(t/1000));
   }

   if(tx_value_last!=0)
   {
      long d=tx_value_now - tx_value_last;
      double t = tx_time_now - tx_time_last;

      txbps=(float)(d/(t/1000));
   }

   tx_value_last=tx_value_now;
   rx_value_last=rx_value_now;

   tx_time_last=tx_time_now;
   rx_time_last=rx_time_now;

   DEBUG_SECTION mea_log_printf("%s (%s) : rx=%.2fbps tx=%.2fbps\n", INFO_STR, __func__, rxbps, txbps);

   *rx=rxbps;
   *tx=txbps;

   return 0;
}


struct _stats_thread_data_s
{
   char essid[40];
   char iface[40];
   struct mini_display_s *display;
   int myID;
};


void *_stats_thread(void *args)
{
   char iface[40]="wlan0";
   char essid[40]="";
   struct mini_display_s *display=NULL;
   int myID;

   struct _stats_thread_data_s *_args=(struct _stats_thread_data_s *)args;

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

   float q, l;
   float qmax, qmin, lmax, lmin;

   mea_timer_t wifi_qual_timer;
   mea_timer_t bps_timer;

   mea_init_timer(&wifi_qual_timer, 5, 1); // quality wifi toutes les 10 secondes
   mea_init_timer(&bps_timer, 1, 1);

   mea_start_timer(&wifi_qual_timer);
   mea_start_timer(&bps_timer);

   if(display)
   {
      mini_display_clear_screen(display,DISPLAY_PAGE);
      mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 1, "rx: _____ tx: _____");
      mini_display_xy_printf(display, DISPLAY_PAGE, 0, 1, 1, "sq: _____ sl: _____");
   }

   int ip_flag, ip_reset_flag=1;
   while(1)
   {
      ip_flag=mea_waitipaddr(iface, 1, NULL);
      if(ip_flag==0)
      {
         if(mea_test_timer(&bps_timer)==0)
         {
            float rx,tx;
            iface_stats(iface, &rx, &tx);
            if(display)
            {
               if(rx>=0 && tx>=0)
                  if(display)
                     mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 1, "rx:%6.1f tx:%6.1f", rx/1024.0, tx/1024.0);
               else
                  if(display)
                     mini_display_xy_printf(display, DISPLAY_PAGE, 0, 0, 1, "rx:%6.1f tx:%6.1f", 0.0, 0.0);
            }
         }

         if( mea_test_timer(&wifi_qual_timer)==0)
         {
            float q,l;
            wifi_connection_status(iface, &q, &l);
            DEBUG_SECTION mea_log_printf("%s (%s) : current link quality - %2.0f%%/%2.0fdb\n",DEBUG_STR,__func__,q,l);
            if(display)
               mini_display_xy_printf(display, DISPLAY_PAGE, 0, 1, 1, "sq:%6.1f sl:%6.1f", q, l);
         }
         ip_reset_flag=1;
      }
      else
      {
         if(ip_reset_flag==1)
         {
            if(display)
            {
               mini_display_clear_screen(display, DISPLAY_PAGE);
               mini_display_xy_printf(display, DISPLAY_PAGE, 3, 1, 1, "NO  CONNECTION");
            }
            ip_reset_flag=0;
         }
      }

      process_heartbeat(myID);

      usleep(1000*1000); // 5 secondes
   }
}


void stats_thread_set_display_page(int page)
{
   stats_display_page=(char)page;
}


pthread_t *stats_thread_start(int myID, char *iface, char *essid, struct mini_display_s *display)
{
   pthread_t *thread=NULL;

   struct _stats_thread_data_s *args=(struct _stats_thread_data_s *)malloc(sizeof(struct _stats_thread_data_s));
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

   if(pthread_create(thread, NULL, (void *)_stats_thread, (void *)args))
   {
      FREE(args);
      args=NULL;
      return NULL;
   }

   pthread_detach(*thread);

   return thread;
}


int stop_statsServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   VERBOSE(5) mea_log_printf("%s (%s) : stop statistics server\n", INFO_STR, __func__);

   if(_statsServer_thread)
   {
      pthread_cancel(*_statsServer_thread);

      usleep(100*1000);

      FREE(_statsServer_thread);
      _statsServer_thread=NULL;
   }

   VERBOSE(5) mea_log_printf("%s (%s) : statistics server stopped\n", INFO_STR, __func__);

   return 0;
}


int start_statsServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct statsServerData_s *statsServerData = (struct statsServerData_s *)data;

   char *iface=statsServerData->iface;
   char *essid=statsServerData->essid;
   struct mini_display_s *display=statsServerData->display;

   VERBOSE(5) mea_log_printf("%s (%s) : start statistics server\n", INFO_STR, __func__);

   _statsServer_thread = stats_thread_start(my_id, iface, essid, display);
   if(_statsServer_thread)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : statistics server started\n", INFO_STR, __func__);
      return 0;
   }
   else
   {
      VERBOSE(5) mea_log_printf("%s (%s) : statistics server not started\n", INFO_STR, __func__);
      return -1;
   }
}


int restart_statsServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;

   ret=stop_statsServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_statsServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}

