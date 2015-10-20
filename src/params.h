#ifndef __params_h
#define __params_h

#include "cfgfile_utils.h"

enum user_params_id_e {
   ADMIN_PASSWORD_ID=0,
   FREE_ID_ID,
   FREE_PASSWORD_ID,
   MY_ESSID_ID,
   MY_PASSWORD_ID,
   NB_USERPARAMS
};

enum sys_params_id_e {
   CONNECTIONCFGFILE_ID=0,
   LCDCFGFILE_ID,
   HOSTAPD_IFACE_ID,
   HOSTAPD_CFG_TEMPLATE_ID,
   IFACE_INTERNET_ID,
   ESSID_INTERNET_ID,
   WWW_PATH_ID,
   SSLPEM_PATH_ID,
   HTTP_PORT_ID,
   HTTPS_PORT_ID,
   LCD_DISPLAY_ENABLED_ID,
   GPIO_BUTTONS_ENABLED_ID,
   GPIO_BUTTON1_ID,
   GPIO_BUTTON2_ID,
   LOG_FILE_ID,
   NB_SYSPARAMS
};


extern struct param_s user_params_keys_list[];
extern struct param_s sys_params_keys_list[];
extern char *user_params[];
extern char *sys_params[];

#endif