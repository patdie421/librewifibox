#include <stdio.h>

#include "params.h"

struct param_s user_params_keys_list[]={
   {"admin_password",     ADMIN_PASSWORD_ID, "admin"},
   {"free_id",            FREE_ID_ID, ""},
   {"free_password",      FREE_PASSWORD_ID, ""},
   {"my_essid",           MY_ESSID_ID, ""},
   {"my_password",        MY_PASSWORD_ID, "0123456789"},
   {"my_ip_and_netmask",  MY_IP_AND_NETMASK_ID, "192.168.10.1, 255.255.255.0"},
   {"my_dhcp_range",      MY_DHCP_RANGE_ID, "192.168.10.100, 192.168.10.200"},
   {NULL,0,NULL}
};

struct param_s sys_params_keys_list[]={
   {"connection_cfgfile", CONNECTIONCFGFILE_ID, "/usr/local/librewifi/etc/user.cfg" },
   {"lcd_cfgfile", LCDCFGFILE_ID, "/usr/local/librewfifi/etc/lcd.cfg" },
   {"hostapd_iface", HOSTAPD_IFACE_ID, "wlan0"},
   {"hostapd_cfg_template", HOSTAPD_CFG_TEMPLATE_ID, "/usr/local/librewifi/etc/hostapd.conf.template" },
   {"essid_internet", ESSID_INTERNET_ID, "FreeWifi"},
   {"iface_internet", IFACE_INTERNET_ID, "wlan1"},
   {"www_path", WWW_PATH_ID, "/usr/local/librewifi/www"},
   {"sslpem_path", SSLPEM_PATH_ID, "/usr/local/librewifi/etc/ssl.pem"},
   {"http_port", HTTP_PORT_ID, "80"},
   {"https_port", HTTPS_PORT_ID, "7443"},
   {"lcd_display_enabled", LCD_DISPLAY_ENABLED_ID, "yes"},
   {"gpio_buttons_enabled", GPIO_BUTTONS_ENABLED_ID, "yes"},
   {"gpio_button1", GPIO_BUTTON1_ID, "4"},
   {"gpio_button2", GPIO_BUTTON2_ID, "5"},
   {"log_file", LOG_FILE_ID, "/var/log/librewifi.log" },
   {"interfaces_template", INTERFACES_TEMPLATE_ID, "/usr/local/librewifi/etc/interfaces.template" },
   {"udhcpd_cfg_template", UDHCPD_CFG_TEMPLATE_ID, "/usr/local/librewifi/etc/udhcpd.conf.template" },
   {NULL,0,NULL}
};

char *user_params[NB_USERPARAMS];
char *sys_params[NB_SYSPARAMS];
