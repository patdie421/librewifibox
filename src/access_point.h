#ifndef __access_point_h
#define __access_point_h

int restart_hostapd_service();
int reboot();
int create_hostapd_cfg(char *template_file, char *dest_file, char *iface, char *essid, char *passwd);
int create_interface_cfg(char *template_file, char *dest_file, char *my_ip, char *my_netmask);

#endif
