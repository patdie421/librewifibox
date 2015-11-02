#ifndef __guiServer_h
#define __guiServer_h


struct guiServerData_s
{
   char *addr;
   char home[256];
   char sslpem[256];
   int http_port;
   int https_port;
   char user_params_file[256];
};

int set_reboot_flag(int flag);
int start_guiServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_guiServer(int my_id, void *data, char *errmsg, int l_errmsg);
int gethttp(char *home, int port);
int restart_guiServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
