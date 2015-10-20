#ifndef __link_h
#define __link_h

#include "minidisplay.h"

struct linkServerData_s
{
   int myID;
   char iface[40];
   char essid[40];
   char free_passwd[20];
   char free_id[10];
   struct mini_display_s *display;
};

void link_thread_set_display_page(int page);

int start_linkServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_linkServer(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_linkServer(int my_id, void *data, char *errmsg, int l_errmsg);

int ifupdown(int myID, char *iface, int updown);

#endif
