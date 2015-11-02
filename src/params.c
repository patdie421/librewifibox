#include <stdio.h>
#include <string.h>

#include "mea_cfgfile.h"
#include "params.h"

/*
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
*/

//struct cfgfile_params_s *user_params2 = NULL;
//struct cfgfile_params_s *sys_params2 = NULL;

char user_params_keys_list_str[256]="";
char *init_user_params_keys_list_str()
{
   sprintf(user_params_keys_list_str,
      "%d:admin_password=admin;"
      "%d:free_id=\"\";"
      "%d:free_password=\"\";"
      "%d:my_essid=\"\";"
      "%d:my_password=\"\";"
      "%d:my_ip_and_netmask=192.168.10.1, 255.255.255.0;"
      "%d:my_dhcp_range=192.168.10.100, 192.168.10.200",
      ADMIN_PASSWORD_ID,
      FREE_ID_ID,
      FREE_PASSWORD_ID,
      MY_ESSID_ID,
      MY_PASSWORD_ID,
      MY_IP_AND_NETMASK_ID,
      MY_DHCP_RANGE_ID);
}


char *get_user_params_keys_list_str()
{
   if(strlen(user_params_keys_list_str)==0)
      init_user_params_keys_list_str();
   return user_params_keys_list_str;
}


char sys_params_keys_list_str[2048]="";
char *init_sys_params_keys_list_str()
{
   sprintf(sys_params_keys_list_str,
      "%d:connection_cfgfile=\"/usr/local/librewifi/etc/user.cfg\";"
      "%d:lcd_cfgfile=\"/usr/local/librewfifi/etc/lcd.cfg\";"
      "%d:hostapd_iface=\"wlan0\";"
      "%d:hostapd_cfg_template=\"/usr/local/librewifi/etc/hostapd.conf.template\";"
      "%d:essid_internet=\"FreeWifi\";"
      "%d:iface_internet=\"wlan1\";"
      "%d:www_path=\"/usr/local/librewifi/www\";"
      "%d:sslpem_path=\"/usr/local/librewifi/etc/ssl.pem\";"
      "%d:http_port=\"80\";"
      "%d:https_port=\"7443\";"
      "%d:lcd_display_enabled=\"yes\";"
      "%d:gpio_buttons_enabled=\"yes\";"
      "%d:gpio_button1=\"4\";"
      "%d:gpio_button2=\"5\";"
      "%d:log_file=/var/log/librewifi.log\";"
      "%d:interfaces_template=\"/usr/local/librewifi/etc/interfaces.template\";"
      "%d:udhcpd_cfg_template=\"/usr/local/librewifi/etc/udhcpd.conf.template\";"
      "%d:verbose_level=\"1\"",
      CONNECTIONCFGFILE_ID,
      LCDCFGFILE_ID,
      HOSTAPD_IFACE_ID,
      HOSTAPD_CFG_TEMPLATE_ID,
      ESSID_INTERNET_ID,
      IFACE_INTERNET_ID,
      WWW_PATH_ID,
      SSLPEM_PATH_ID,
      HTTP_PORT_ID,
      HTTPS_PORT_ID,
      LCD_DISPLAY_ENABLED_ID,
      GPIO_BUTTONS_ENABLED_ID,
      GPIO_BUTTON1_ID,
      GPIO_BUTTON2_ID,
      LOG_FILE_ID,
      INTERFACES_TEMPLATE_ID,
      UDHCPD_CFG_TEMPLATE_ID,
      VERBOSE_LEVEL_ID);

   return sys_params_keys_list_str;
};


char *get_sys_params_keys_list_str()
{
   if(strlen(sys_params_keys_list_str)==0)
      init_sys_params_keys_list_str();
   return sys_params_keys_list_str;
}

