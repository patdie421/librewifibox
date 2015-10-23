#ifndef __access_point_h
#define __access_point_h

int restart_hostapd_service();
int reboot();
int create_hostapd_cfg(char *template_file, char *dest_file, char *iface, char *essid, char *passwd);
int create_interfaces_cfg(char *template_file, char *dest_file, char *ip_and_netmask);
int create_udhcpd_cfg(char *template_file, char *dest_file, char *iface, char *ip_and_netmask, char *dhcp_range);

#endif
