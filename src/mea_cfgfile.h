#ifndef __mea_cfgfile_utils_h
#define __mea_cfgfile_utils_h

#include <inttypes.h>

struct cfgfile_keyvalue_s
{
   char *key;
   int id;
   char *default_value;
};


int mea_load_cfgfile(char *file, struct cfgfile_keyvalue_s *keys_values_list, char *params[], uint16_t nb_params);
int mea_write_cfgfile(char *file, struct cfgfile_keyvalue_s *keys_values_list, char *params[], uint16_t nb_params);
int mea_clean_cfgfile(char *params[], uint16_t nb_params);
int mea_create_file_from_template(char *template_file, char *dest_file, ...);

#endif
