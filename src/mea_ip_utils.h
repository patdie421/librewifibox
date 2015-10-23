#ifndef __mea_ip_utils_h
#define __mea_ip_utils_h

int mea_getifaceaddr(char *iface, char *addr);
int mea_ping(char *address);
int mea_waitipaddr(char *iface, int timeout, char *addr);
int mea_isnetmask(uint32_t netmask);
int mea_areaddrsinsamenetwork(uint32_t addr1, uint32_t addr2, uint32_t netmask);
int mea_strisvalidaddr(char *s, uint32_t *addr);
int mea_strisvalidnetmask(char *s, uint32_t *n);

#endif
