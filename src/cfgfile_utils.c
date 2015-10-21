#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfgfile_utils.h"
#include "string_utils.h"
#include "mea_verbose.h"

int _getParamID(struct param_s params_list[], char *str)
{
   if(!str)
      return -1;
 
   uint16_t i;
   for(i=0;params_list[i].key;i++)
   {
      if(mea_strcmplower(params_list[i].key, str) == 0)
         return params_list[i].id;
   }
   return -1;
}


char *_getParamKey(struct param_s params_list[], int id)
{
   uint16_t i;
   for(i=0;params_list[i].key;i++)
   {
      if(params_list[i].id == id)
         return params_list[i].key;
   }
   return NULL;
}


int mea_clean_cfgfile(char *params[], uint16_t nb_params)
{
   int i=0;
   for(;i<nb_params;i++)
   {
      if(params[i])
      {
         free(params[i]);
         params[i]=NULL;
      }
   }
   return 0;
}


int mea_write_cfgfile(char *file, struct param_s *keys_names_list, char *params[], uint16_t nb_params)
{
   int ret_code=0;
   FILE *fd;

   fd=fopen(file, "w");
   if(!fd)
   {
      perror("");
      return -1;
   }

   int i=0;
   for(;i<nb_params;i++)
   {
      int j=0;
      for(;j<nb_params;j++)
      {
         if(keys_names_list[j].id==i)
         {
            fprintf(fd,"%s = %s\n", keys_names_list[j].key, params[i]);
            DEBUG_SECTION mea_log_printf("%s = %s\n", keys_names_list[j].key, params[i]);
         }
      }
   } 

mea_write_cfgfile_clean_exit:
   fclose(fd);
   return ret_code;
}


int mea_load_cfgfile(char *file, struct param_s *keys_names_list, char *params[], uint16_t nb_params)
{
   FILE *fd;
 
   fd=fopen(file, "r");
   if(!fd)
   {
      perror("");
      return -1;
   }
 
   int i=0;
   for(i=0;i<nb_params;i++)
   {
      if(params[i])
      {
         free(params[i]);
         params[i]=NULL;
      }
      params[i]=(char *)malloc(strlen(keys_names_list[i].default_value)+1);
      if(params[i]==NULL)
      {
         fprintf(stderr,"error - malloc error\n");
         goto mea_load_config_file_clean_exit;
      }
      strcpy(params[i],keys_names_list[i].default_value);
   }
 
   char line[255];
   char *nocomment[2];
   char key[40], value[40];
   char *tkey,*tvalue;
 
   i=0;
   while(!feof(fd))
   {
      if(fgets(line,sizeof(line),fd) != NULL)
      {
         i++;
         // suppression du commentaire éntuellement présent dans la ligne
         mea_strsplit(line, '#', nocomment, 2);
         char *keyvalue=mea_strtrim(nocomment[0]);
         if(strlen(keyvalue)==0)
            continue;
 
         // lecture d'un couple key/value
         key[0]=0;
         value[0]=0;
         sscanf(keyvalue,"%[^=]=%[^\n]", key, value);
 
         if(key[0]==0 || value[0]==0)
         {
            VERBOSE(5) mea_log_printf("syntax error : %s, line %d not a \"key = value\" line\n", file, i);
            goto mea_load_config_file_clean_exit;
         }
         
         char *tkey=mea_strtrim(key);
         char *tvalue=mea_strtrim(value);
         mea_strtolower(tkey);
//         mea_strtolower(tvalue);

         int id;
         id=_getParamID(keys_names_list, tkey);
         if(id<0)
         {
            mea_log_printf("warning : \"%s\" unknown parameter\n",tkey);
            continue;
         }

         if(params[id])
            free(params[id]); 
         params[id]=(char *)malloc(strlen(tvalue)+1);
         if(params[id]==NULL)
         {
            mea_log_printf("error - malloc error\n");
            goto mea_load_config_file_clean_exit;
         }

         strcpy(params[id],tvalue);
      }
   }
 
   return 0;
   
mea_load_config_file_clean_exit:
   for(i=0;i<nb_params;i++)
   {
      if(params[i])
      {
         free(params[i]);
         params[i]=NULL;
      }
   }

   return -1;
}
