#ifndef __ip_utils_h
#define __ip_utils_h

int getifaceaddr(char *iface, char *addr);
int ping(char *address);
int waitipaddr(char *iface, int timeout, char *addr);

#endif
