#ifndef __scanner_h
#define __scanner_h

#include <pthread.h>

struct scannerServerData_s
{
   int myID;
   char iface[40];
   char essid[40];
   struct mini_display_s *display;
};

extern char scanner_flag;
void scanner_on();
void scanner_off();
void scanner_thread_set_display_page(int page);

int start_scannerServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_scannerServer(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_scannerServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
