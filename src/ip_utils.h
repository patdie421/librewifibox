#ifndef __ip_utils_h
#define __ip_utils_h

int getifaceaddr(char *iface, char *addr);
int ping(char *address);
int waitipaddr(char *iface, int timeout, char *addr);
int is_netmask(uint32_t netmask);
int addrs_in_same_network(uint32_t addr1, uint32_t addr2, uint32_t netmask);
int str_is_valid_addr(char *s, uint32_t *addr);
int str_is_valid_netmask(char *s, uint32_t *n);

#endif
