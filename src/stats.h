#ifndef __stats_h
#define __stats_h

#include "minidisplay.h"

struct statsServerData_s
{
   int myID;
   char iface[40];
   char essid[40];
   struct mini_display_s *display;
};

void stats_thread_set_display_page(int page);

int start_statsServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_statsServer(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_statsServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
