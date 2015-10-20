#ifndef __hostapd_h
#define __hostapd_h

int restart_hostapd_service();
int reboot();
int populate_hostapd_cfg(char *template_file, char *dest_file, char *iface, char *essid, char *passwd);

#endif
