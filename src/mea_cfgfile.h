#ifndef __mea_cfgfile_utils_h
#define __mea_cfgfile_utils_h

#include <inttypes.h>

struct cfgfile_keyvalue_s
{
   char *key;
   int id;
   char *default_value;
   char *value;
};

struct cfgfile_params_s
{
   uint16_t nb_params;
   struct cfgfile_keyvalue_s *keysvalues_list;
};

struct cfgfile_params_s
     *mea_cfgfile_params_alloc(struct cfgfile_keyvalue_s *keys_values_list);
int   mea_cfgfile_load(char *file, struct cfgfile_params_s *cfgfile_params);
int   mea_cfgfile_write(char *file, struct cfgfile_params_s *cfgfile_params);
int   mea_cfgfile_clean(struct cfgfile_params_s *cfgfile_params);
char *mea_cfgfile_get_value_by_id(struct cfgfile_params_s *cfgfile_params, int id);
int   mea_cfgfile_set_value_by_id(struct cfgfile_params_s *cfgfile_params, int id, char *value);
int   mea_create_file_from_template(char *template_file, char *dest_file, ...);
void  mea_cfgfile_print(struct cfgfile_params_s *cfgfile_params);

#endif
