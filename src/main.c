#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <poll.h>
#include <signal.h>

#include <curl/curl.h>
#include "iwlib.h"
#include "liblcd/lcd.h"

#include "mea_verbose.h"
#include "mea_timer.h"
#include "mea_gpio.h"
#include "mea_string_utils.h"
#include "mea_cfgfile.h"

#include "processManager.h"
#include "minidisplay.h"
#include "params.h"

#include "access_point.h"
#include "scanner.h"
#include "link.h"
#include "stats.h"
#include "gui.h"


int linkServer_monitoring_id=-1;
int statsServer_monitoring_id=-1;
int scannerServer_monitoring_id=-1;
int guiServer_monitoring_id=-1;

int buttons_enabled=-1;
int button1=0, button2=0;
int button1_fd=-1;
int button2_fd=-1;

char *syscfg_file;
char *log_file=NULL;

struct lcd_s *lcd=NULL;
struct mini_display_s *display=NULL;


int usage(char *cmnd)
{
   fprintf(stderr,"usage : %s cfgfile\n");

   return -1;
}


int logfile_rotation_job(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(log_file)
   {
      mea_log_printf("%s (%s) : job rotation start\n", INFO_STR, __func__);
      mea_rotate_open_log_file(MEA_STDERR, log_file, 6);
      mea_log_printf("%s (%s) : job rotation done\n", INFO_STR, __func__);
   }
   return;
}


void stop_all(int exit_code)
{
   VERBOSE(1) mea_log_printf("%s (%s) : stopping service\n", INFO_STR, __func__);

   if(linkServer_monitoring_id!=-1)
      process_stop(linkServer_monitoring_id, NULL, 0);

   if(statsServer_monitoring_id!=-1)
      process_stop(statsServer_monitoring_id, NULL, 0);

   if(scannerServer_monitoring_id!=-1)
      process_stop(scannerServer_monitoring_id, NULL, 0);

    if(guiServer_monitoring_id!=-1)
       stop_guiServer(guiServer_monitoring_id, NULL, NULL, 0);

   if(display)
   {
      mini_display_release(display);
      free(display);
      display=NULL;
   }

   if(lcd)
   {
      lcd_release(lcd);
      lcd_free(&lcd);
   }

   if(buttons_enabled==0)
   {
      if(button1_fd!=-1)
      {
         mea_gpio_fd_close(button1_fd);
         mea_gpio_unexport(button1);
      }

      if(button2_fd!=-1)
      {
         mea_gpio_fd_close(button2_fd);
         mea_gpio_unexport(button2);
      }
   }

   mea_clean_cfgfile(sys_params, NB_SYSPARAMS);
   mea_clean_cfgfile(user_params, NB_USERPARAMS);

   if(syscfg_file)
   {
      free(syscfg_file);
      syscfg_file=NULL;
   }

   if(log_file)
   {
      free(log_file);
      log_file=NULL;
   }

   VERBOSE(1) mea_log_printf("%s (%s) : bye\n", INFO_STR, __func__);

   exit(exit_code);
}


void sigterm_handler(int signo)
{
   stop_all(0);
}


void sighup_handler(int signo)
{
   if (signo == SIGHUP) // demande de rechargement de la configuration
   {
      char free_id_prev[20];
      char free_passwd_prev[32];
      char hostapd_essid_prev[IW_ESSID_MAX_SIZE+1];
      char hostapd_passwd_prev[64];
      char ip_netmask_prev[64];
      char dhcp_start_end_prev[64];


      VERBOSE(2) mea_log_printf("%s (%s) : reload new parameters\n", INFO_STR, __func__);

      // sauvegarde temporaire des anciens valeurs
      strncpy(free_id_prev, user_params[FREE_ID_ID],sizeof(free_id_prev));
      strncpy(free_passwd_prev, user_params[FREE_PASSWORD_ID], sizeof(free_passwd_prev));
      strncpy(hostapd_essid_prev, user_params[MY_ESSID_ID], sizeof(hostapd_essid_prev));
      strncpy(hostapd_passwd_prev, user_params[MY_PASSWORD_ID], sizeof(hostapd_passwd_prev));
      strncpy(ip_netmask_prev, user_params[MY_IP_AND_NETMASK_ID], sizeof(ip_netmask_prev));
      strncpy(dhcp_start_end_prev, user_params[MY_DHCP_RANGE_ID], sizeof(dhcp_start_end_prev));

      // chargement des nouvelles valeurs
      int ret=mea_load_cfgfile(sys_params[CONNECTIONCFGFILE_ID], user_params_keys_list, user_params, NB_USERPARAMS);
      if(ret<0)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : can't reload cfg file\n", ERROR_STR, __func__);
         set_reboot_flag(0);
         return;
      }

     
      if( strcmp(hostapd_essid_prev,  user_params[MY_ESSID_ID])!=0 ||
          strcmp(hostapd_passwd_prev, user_params[MY_PASSWORD_ID])!=0 ||
          strcmp(ip_netmask_prev,     user_params[MY_IP_AND_NETMASK_ID])!=0 ||
          strcmp(dhcp_start_end_prev, user_params[MY_DHCP_RANGE_ID])!=0)
      {
          set_reboot_flag(1); // pour que gui affiche une page d'info reboot

          // reconfiguration des fichiers systeme
          create_hostapd_cfg(sys_params[HOSTAPD_CFG_TEMPLATE_ID], "/etc/hostapd/hostapd.conf", sys_params[HOSTAPD_IFACE_ID], user_params[MY_ESSID_ID], user_params[MY_PASSWORD_ID]);
          create_interfaces_cfg(sys_params[INTERFACES_TEMPLATE_ID], "/etc/network/interfaces", user_params[MY_IP_AND_NETMASK_ID]);
          create_udhcpd_cfg(sys_params[UDHCPD_CFG_TEMPLATE_ID], "/etc/udhcpd.conf", sys_params[HOSTAPD_IFACE_ID], user_params[MY_IP_AND_NETMASK_ID], user_params[MY_DHCP_RANGE_ID]);

          VERBOSE(2) mea_log_printf("%s (%s) : new parameters need a reboot\n", INFO_STR, __func__);

          sleep(5);

          reboot();

          stop_all(0);
      }
      else
          set_reboot_flag(0); // pas besoin de rebooter pour les autres changements de config

      if( strcmp(free_id_prev, user_params[FREE_ID_ID])!=0 ||
          strcmp(free_passwd_prev, user_params[FREE_PASSWORD_ID])!=0)
      {
         process_stop(linkServer_monitoring_id, NULL, 0);

         ifupdown(linkServer_monitoring_id, sys_params[IFACE_INTERNET_ID], 0);

         struct linkServerData_s *linkServer_start_stop_params;
         linkServer_start_stop_params=process_get_data_ptr(linkServer_monitoring_id);
         if(linkServer_start_stop_params)
         {
            strncpy(linkServer_start_stop_params->free_passwd, user_params[FREE_PASSWORD_ID], sizeof(linkServer_start_stop_params->free_passwd));
            strncpy(linkServer_start_stop_params->free_id, user_params[FREE_ID_ID], sizeof(linkServer_start_stop_params->free_id));
         }
 
         process_start(linkServer_monitoring_id, NULL, 0);
      }

      VERBOSE(2) mea_log_printf("%s (%s) : new parameters reloaded\n", INFO_STR, __func__);
   }
}


int main(int argc, char *argv[])
{
   int ret=0;
   int exit_code=0;

   char *internet_iface=NULL;
   char *internet_essid=NULL;
   char *free_id=NULL;
   char *free_password=NULL;
   int http_port=-1, https_port=-1;

   char lan_addr[16];

   mea_timer_t pmgr_timer; // process manager timer
   mea_timer_t pages_timer;
   mea_timer_t bl_timer; // lcd display backlight timer

   uint16_t dpage=0;

   int toggle_button_state=-1;
   int restart_button_state=-1;
   int backlight_state=1;


   //
   // chargement des parametres de l'application
   //
   if(argc!=2)
   {
      usage(argv[0]);
      exit(1);
   }

   syscfg_file=(char *)malloc(strlen(argv[1])+1);
   if(syscfg_file==NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't alloc memory\n", ERROR_STR, __func__);
      exit(1);
   }
   strcpy(syscfg_file,argv[1]);

   // parametres system
   memset(sys_params, 0, sizeof(char *)*NB_SYSPARAMS);
   ret=mea_load_cfgfile(syscfg_file, sys_params_keys_list, sys_params, NB_SYSPARAMS);
   if(ret<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't load system configuration file\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   // parametres utilisateur
   memset(user_params, 0, sizeof(char *)*NB_USERPARAMS);
   ret=mea_load_cfgfile(sys_params[CONNECTIONCFGFILE_ID], user_params_keys_list, user_params, NB_USERPARAMS);
   if(ret<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't load user configuration file\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }


   //
   // récupération des informations nécessaires au fonctionnement
   //
   internet_essid=sys_params[ESSID_INTERNET_ID];
   internet_iface=sys_params[IFACE_INTERNET_ID];
   free_id=user_params[FREE_ID_ID];
   free_password=user_params[FREE_PASSWORD_ID];

   // adresse ip de l'interface LAN
   if(mea_getifaceaddr(sys_params[HOSTAPD_IFACE_ID], lan_addr)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : no local addr on \"%s\" interface\n", ERROR_STR, __func__, sys_params[HOSTAPD_IFACE_ID]);
      exit_code=1;
      goto main_clean_exit;
   }

   // ports des services http(s)
   if(mea_strisnumeric(sys_params[HTTP_PORT_ID])==0)
      http_port=atoi(sys_params[HTTP_PORT_ID]);
   else
   {
      struct servent *serv = getservbyname(sys_params[HTTP_PORT_ID], "tcp"); 
      if(serv)
         http_port=ntohs(serv->s_port);
   }
   if(http_port < 0 || http_port > 0xFFFF)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : incorrect http port (%s)\n", ERROR_STR, __func__, sys_params[HTTP_PORT_ID]);
      exit_code=1;
      goto main_clean_exit;
   }

   if(mea_strisnumeric(sys_params[HTTPS_PORT_ID])==0)
      https_port=atoi(sys_params[HTTPS_PORT_ID]);
   else
   {
      struct servent *serv = getservbyname(sys_params[HTTPS_PORT_ID], "tcp"); 
      if(serv)
         https_port=ntohs(serv->s_port);
   }
   if(http_port < 0 || http_port > 0xFFFF)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : incorrect https port (%s)\n", ERROR_STR, __func__, sys_params[HTTPS_PORT_ID]);
      exit_code=1;
      goto main_clean_exit;
   }


   // affichage LCD
   if(strcmp(sys_params[LCD_DISPLAY_ENABLED_ID],"yes")==0)
   {
      lcd = lcd_form_cfgfile_alloc(sys_params[LCDCFGFILE_ID]);
      if(lcd == NULL)
      {
         VERBOSE(2) {
            mea_log_printf("%s (%s) : can't load display configuration - ",ERROR_STR,__func__);
            perror("");
         }
      }

      if(lcd && lcd_init(lcd, 0)<0)
      {
         VERBOSE(2) {
            mea_log_printf("%s (%s) : can't init display device - ",ERROR_STR,__func__);
            perror("");
         }
      }

      display=(struct mini_display_s *)malloc(sizeof(struct mini_display_s));
      if(display)
      {
         if(mini_display_set_driver(display, LCD, lcd)<0)
         {
            VERBOSE(2) {
               mea_log_printf("%s (%s) : can't connect to display driver\n",ERROR_STR,__func__);
            }
         }
         mini_display_init(display, 5, 20, 4);

         mini_display_change_page(display, dpage);
      }
      else
      {
         VERBOSE(5) {
            mea_log_printf("%s (%s) : can't alloc memory - ",ERROR_STR,__func__);
            perror("");
         }
         lcd_release(lcd);
         lcd_free(&lcd);
      }
   }


   // boutons GPIO
   if(strcmp(sys_params[GPIO_BUTTONS_ENABLED_ID],"yes")==0)
   {
      buttons_enabled=0;

      if(mea_strisnumeric(sys_params[GPIO_BUTTON1_ID])==0)
      {
         button1=atoi(sys_params[GPIO_BUTTON1_ID]);
         mea_gpio_export(button1);
         mea_gpio_set_dir(button1, 0); // en entrée
         mea_gpio_set_edge(button1, "both");
         button1_fd=mea_gpio_fd_open(button1);
      }
      else
         button1=-1;

      if(mea_strisnumeric(sys_params[GPIO_BUTTON2_ID])==0)
      {
         button2=atoi(sys_params[GPIO_BUTTON2_ID]);
         mea_gpio_export(button2);
         mea_gpio_set_dir(button2, 0); // en entrée
         button2_fd=mea_gpio_fd_open(button2);
      }
      else
         button2=-1;
   }

   // fichier de log
   log_file=(char *)malloc(strlen(sys_params[LOG_FILE_ID]+1));
   if(log_file==NULL)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't alloc memory - ",ERROR_STR,__func__);
         perror("");
      }
      exit_code=1;
      goto main_clean_exit;
   }
   strcpy(log_file,sys_params[LOG_FILE_ID]);

   int fd=open(log_file, O_CREAT | O_APPEND | O_RDWR, S_IWUSR | S_IRUSR);
   if(fd<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't open log file - ",ERROR_STR,__func__);
      perror("");
      exit_code=1;
      goto main_clean_exit;
   }
   dup2(fd, 1);
   dup2(fd, 2);
   close(fd);

   //
   // initialisation de la gestion des signaux
   //
   if (signal(SIGHUP, sighup_handler) == SIG_ERR)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't init SIGHUP handler\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't init SIGTERM handler\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   if (signal(SIGINT, sigterm_handler) == SIG_ERR)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't init SIGINT handler\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   if (signal(SIGQUIT, sigterm_handler) == SIG_ERR)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't init SIGQUIT handler\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   //
   // préparation des timers
   //
   mea_init_timer(&pmgr_timer, 1, 1);
   mea_init_timer(&bl_timer, 10, 0);
   mea_init_timer(&pages_timer, 15, 1);

   mea_start_timer(&pmgr_timer);
   mea_start_timer(&bl_timer);
   mea_start_timer(&pages_timer);

   link_thread_set_display_page(0);
   stats_thread_set_display_page(1);
   scanner_thread_set_display_page(2);

   init_processes_manager(1);

   //
   // lancement des threads et process
   //
   struct guiServerData_s guiServer_start_stop_params;
   guiServer_start_stop_params.addr=lan_addr;
   strncpy(guiServer_start_stop_params.home, sys_params[WWW_PATH_ID], sizeof(guiServer_start_stop_params.home));
   strncpy(guiServer_start_stop_params.sslpem, sys_params[SSLPEM_PATH_ID], sizeof(guiServer_start_stop_params.sslpem));
   guiServer_start_stop_params.http_port=http_port;
   guiServer_start_stop_params.https_port=https_port;
   guiServer_monitoring_id=process_register("guiServer");
   if(start_guiServer(guiServer_monitoring_id, (void *)(&guiServer_start_stop_params), NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start gui server\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   struct linkServerData_s linkServer_start_stop_params;
   strcpy(linkServer_start_stop_params.iface, internet_iface);
   strcpy(linkServer_start_stop_params.essid, internet_essid);
   strcpy(linkServer_start_stop_params.free_passwd, free_password);
   strcpy(linkServer_start_stop_params.free_id, free_id);
   linkServer_start_stop_params.display=display;
   linkServer_monitoring_id=process_register("linkServer");
   process_set_start_stop(linkServer_monitoring_id, start_linkServer, stop_linkServer, (void *)(&linkServer_start_stop_params), 1);
   process_set_watchdog_recovery(linkServer_monitoring_id, restart_linkServer, (void *)(&linkServer_start_stop_params));
   process_set_heartbeat_interval(linkServer_monitoring_id, 60);
   process_add_indicator(linkServer_monitoring_id, "NB_RECONNECTION", 0);
   process_index_indicators_list(linkServer_monitoring_id);
   if(process_start(linkServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start link server\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   struct statsServerData_s statsServer_start_stop_params;
   strcpy(statsServer_start_stop_params.iface, internet_iface);
   strcpy(statsServer_start_stop_params.essid, internet_essid);
   statsServer_start_stop_params.display=display;
   statsServer_monitoring_id=process_register("statsServer");
   process_set_start_stop(statsServer_monitoring_id, start_statsServer, stop_statsServer, (void *)(&statsServer_start_stop_params), 1);
   process_set_watchdog_recovery(statsServer_monitoring_id, restart_statsServer, (void *)(&statsServer_start_stop_params));
   process_set_heartbeat_interval(statsServer_monitoring_id, 60);
   if(process_start(statsServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start statistics server\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }

   struct scannerServerData_s scannerServer_start_stop_params;
   strcpy(scannerServer_start_stop_params.iface, internet_iface);
   strcpy(scannerServer_start_stop_params.essid, internet_essid);
   scannerServer_start_stop_params.display=display;
   scannerServer_monitoring_id=process_register("scannerServer");
   process_set_start_stop(scannerServer_monitoring_id, start_scannerServer, stop_scannerServer, (void *)(&scannerServer_start_stop_params), 1);
   process_set_watchdog_recovery(scannerServer_monitoring_id, restart_scannerServer, (void *)(&scannerServer_start_stop_params));
   process_set_heartbeat_interval(scannerServer_monitoring_id, 60);
   if(process_start(scannerServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start scanner server\n",ERROR_STR,__func__);
      exit_code=1;
      goto main_clean_exit;
   }


   int job_id=process_register("LOGROTATION");
   process_set_start_stop(job_id, logfile_rotation_job, NULL, NULL, 1);
   process_set_type(job_id, JOB);
//   process_job_set_scheduling_data(job_id, "0,5,10,15,20,25,30,35,40,45,50,55|*|*|*|*", 0);
   process_job_set_scheduling_data(job_id, "0|13,21,5|*|*|*", 0); // rotation des log 3x par jour
   process_set_group(job_id, 1);

   // au démarrage : rotation des log.
   process_run_task(job_id, NULL, 0);

   for(;;) // boucle principale (gestion des i/o ihm)
   {
      if(buttons_enabled==0)
      {
         mea_gpio_wait_intr(button1_fd, 25*1000);

         toggle_button_state=mea_gpio_get_state(button1_fd);
         restart_button_state=mea_gpio_get_state(button2_fd);

         // gestion du rétroéclairage
         if(toggle_button_state==1)
         {
            usleep(25*1000); // anti-rebond : on reli après 25ms pour être sûr ... ou presque
            toggle_button_state=mea_gpio_get_state(button1_fd);
            if(toggle_button_state==1)
            {
               if(backlight_state==0) // le bouton est appuyé
               {
                  backlight_state=1; // rétroéclairage actif
               }
               else
               {
                  dpage=dpage+1;
                  if(dpage>2)
                     dpage=0;
                  if(dpage==2)
                     scanner_on();
                  else
                     scanner_off();
               }
            }

            mea_start_timer(&bl_timer); // démarrage/reamorçage de la minuterie
         }

         if(mea_test_timer(&bl_timer)==0)
         {
            backlight_state=0;
         }
      }
      else
         usleep(250*1000);

/*
      if(display && mea_test_timer(&pages_timer)==0)
      {
         dpage=dpage+1;
         if(dpage>2)
            dpage=0;
         mini_display_change_page(display, dpage);
         if(dpage==2)
            scanner_on();
         else
            scanner_off();
      }
*/

      if(mea_test_timer(&pmgr_timer)==0)
      {
         managed_processes_loop();
      }
   }

main_clean_exit:
   stop_all(exit_code);
}
