#ifndef __wifi_h
#define __wifi_h

#include "iwlib.h"

#define IW_SCAN_HACK	0x8000

struct ap_data_s
{
   int index;
   char essid[IW_ESSID_MAX_SIZE+1];
   float quality;
   float lvl;
};


int wifi_connection_status(char *iface, float *q, float *l);
struct ap_data_s *ap_data_get_alloc(int skfd, char *ifname, char *searched_essid, int last);
int wifi_ap_find(char *iface, char *essid_searched, float *qmax, float *lmax, float *qmin, float *lmin);
int waitessid(char *iface, char *essid, float min_quality, int timeout);

#endif

