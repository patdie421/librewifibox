#ifndef __cfgfile_utils_h
#define __cfgfile_utils_h

#include <inttypes.h>

struct param_s
{
   char *key;
   int id;
   char *default_value;
};

int mea_load_cfgfile(char *file, struct param_s *keys_names_list, char *params[], uint16_t nb_params);
int mea_write_cfgfile(char *file, struct param_s *keys_names_list, char *params[], uint16_t nb_params);
int mea_clean_cfgfile(char *params[], uint16_t nb_params);

#endif
