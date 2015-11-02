#include <stdio.h>

#include "mea_cfgfile.h"
#include "params.h"

struct cfgfile_keyvalue_s user_params_keys_list[]={
   {"admin_password",     ADMIN_PASSWORD_ID, "admin", NULL},
   {"free_id",            FREE_ID_ID, "", NULL},
   {"free_password",      FREE_PASSWORD_ID, "", NULL},
   {"my_essid",           MY_ESSID_ID, "", NULL},
   {"my_password",        MY_PASSWORD_ID, "0123456789", NULL},
   {"my_ip_and_netmask",  MY_IP_AND_NETMASK_ID, "192.168.10.1, 255.255.255.0", NULL},
   {"my_dhcp_range",      MY_DHCP_RANGE_ID, "192.168.10.100, 192.168.10.200", NULL},
   {NULL,0,NULL,NULL}
};

struct cfgfile_keyvalue_s sys_params_keys_list[]={
   {"connection_cfgfile", CONNECTIONCFGFILE_ID, "/usr/local/librewifi/etc/user.cfg", NULL },
   {"lcd_cfgfile", LCDCFGFILE_ID, "/usr/local/librewfifi/etc/lcd.cfg", NULL },
   {"hostapd_iface", HOSTAPD_IFACE_ID, "wlan0", NULL },
   {"hostapd_cfg_template", HOSTAPD_CFG_TEMPLATE_ID, "/usr/local/librewifi/etc/hostapd.conf.template", NULL },
   {"essid_internet", ESSID_INTERNET_ID, "FreeWifi", NULL },
   {"iface_internet", IFACE_INTERNET_ID, "wlan1", NULL },
   {"www_path", WWW_PATH_ID, "/usr/local/librewifi/www", NULL },
   {"sslpem_path", SSLPEM_PATH_ID, "/usr/local/librewifi/etc/ssl.pem", NULL},
   {"http_port", HTTP_PORT_ID, "80", NULL},
   {"https_port", HTTPS_PORT_ID, "7443", NULL },
   {"lcd_display_enabled", LCD_DISPLAY_ENABLED_ID, "yes", NULL },
   {"gpio_buttons_enabled", GPIO_BUTTONS_ENABLED_ID, "yes", NULL },
   {"gpio_button1", GPIO_BUTTON1_ID, "4", NULL },
   {"gpio_button2", GPIO_BUTTON2_ID, "5", NULL },
   {"log_file", LOG_FILE_ID, "/var/log/librewifi.log", NULL },
   {"interfaces_template", INTERFACES_TEMPLATE_ID, "/usr/local/librewifi/etc/interfaces.template", NULL },
   {"udhcpd_cfg_template", UDHCPD_CFG_TEMPLATE_ID, "/usr/local/librewifi/etc/udhcpd.conf.template", NULL },
   {"verbose_level", VERBOSE_LEVEL_ID, "1", NULL },
   {NULL,0,NULL,NULL}
};

//char *user_params[NB_USERPARAMS];
//char *sys_params[NB_SYSPARAMS];

struct cfgfile_params_s *user_params2 = NULL;
struct cfgfile_params_s *sys_params2 = NULL;
