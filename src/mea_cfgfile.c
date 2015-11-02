#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mea_cfgfile.h"

#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "mea_memfile.h"


static int _getParamID(struct cfgfile_keyvalue_s params_list[], char *str)
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


static char *_getParamKey(struct cfgfile_keyvalue_s keys_values_list[], int id)
{
   uint16_t i;
   for(i=0;keys_values_list[i].key;i++)
   {
      if(keys_values_list[i].id == id)
         return keys_values_list[i].key;
   }
   return NULL;
}


static struct cfgfile_keyvalue_s *_getKeyValue(struct cfgfile_params_s *cfgfile_params, char *key)
{
   if(!key)
      return NULL;
 
   uint16_t i;
   for(i=0;cfgfile_params->keysvalues_list[i].key;i++)
   {
      if(mea_strcmplower(cfgfile_params->keysvalues_list[i].key, key) == 0)
         return &cfgfile_params->keysvalues_list[i];
   }
   return NULL;
}


void mea_cfgfile_print(struct cfgfile_params_s *cfgfile_params)
{
   uint16_t i;
   for(i=0;cfgfile_params->keysvalues_list[i].key;i++)
   {
      if(cfgfile_params->keysvalues_list[i].value)
         fprintf(stderr,"%s = %s\n", cfgfile_params->keysvalues_list[i].key, cfgfile_params->keysvalues_list[i].value);
      else
         fprintf(stderr,"%s = %s\n", cfgfile_params->keysvalues_list[i].key, cfgfile_params->keysvalues_list[i].default_value);
   }
}


char *mea_cfgfile_get_value_by_id(struct cfgfile_params_s *cfgfile_params, int id)
{
   if(id<0 || id >= cfgfile_params->nb_params)
      return NULL;

   int i=0;
   for(;i<cfgfile_params->nb_params;i++)
   {
      if(cfgfile_params->keysvalues_list[i].id == id)
      {
         if(cfgfile_params->keysvalues_list[i].value == NULL)
            return cfgfile_params->keysvalues_list[i].default_value;
         else
            return cfgfile_params->keysvalues_list[i].value;
      }
   }

   return NULL;
}


int mea_cfgfile_set_value_by_id(struct cfgfile_params_s *cfgfile_params, int id, char *value)
{
   if(id<0 || id >= cfgfile_params->nb_params)
      return -1;

   int i=0;
   for(;i<cfgfile_params->nb_params;i++)
   {
      if(cfgfile_params->keysvalues_list[i].id == id)
      {
         char *ptr=(char *)malloc(strlen(value)+1);
         if(ptr==NULL)
            return -1;

         if(cfgfile_params->keysvalues_list[i].value)
            free(cfgfile_params->keysvalues_list[i].value);
         cfgfile_params->keysvalues_list[i].value = ptr;

         strcpy(ptr, value);
      }
   }

   return NULL;
}


struct cfgfile_params_s *mea_cfgfile_params_alloc(struct cfgfile_keyvalue_s *keys_values_list)
{
   if(keys_values_list == NULL)
      return NULL;

   struct cfgfile_params_s *params = (struct cfgfile_params_s *)malloc(sizeof(struct cfgfile_params_s));

   if(params==NULL)
      return NULL;

   int i=0;
   for(;keys_values_list[i].key;i++);
   if(i==0)
   {
      free(params);
      params=NULL;
      return NULL; 
   }

   params->nb_params = i;

   params->keysvalues_list=keys_values_list;

   return params;
}


int mea_cfgfile_clean(struct cfgfile_params_s *cfgfile_params)
{
   if(cfgfile_params == NULL)
      return -1;

   int i=0;
   for(;i<cfgfile_params->nb_params;i++)
   {
      if(cfgfile_params->keysvalues_list[i].value)
      {
         free(cfgfile_params->keysvalues_list[i].value);
         cfgfile_params->keysvalues_list[i].value=NULL;
      }
   }

   return 0;
}


int mea_cfgfile_write(char *file, struct cfgfile_params_s *cfgfile_params)
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
   for(;i<cfgfile_params->nb_params;i++)
   {
      if(cfgfile_params->keysvalues_list[i].value)
      {
         fprintf(fd,"%s = %s\n", cfgfile_params->keysvalues_list[i].key, cfgfile_params->keysvalues_list[i].value);
         DEBUG_SECTION mea_log_printf("%s = %s\n", cfgfile_params->keysvalues_list[i].key, cfgfile_params->keysvalues_list[i].value);
      }
      else
      {
         fprintf(fd,"%s = %s\n", cfgfile_params->keysvalues_list[i].key, cfgfile_params->keysvalues_list[i].default_value);
         DEBUG_SECTION mea_log_printf("%s = %s\n", cfgfile_params->keysvalues_list[i].key, cfgfile_params->keysvalues_list[i].value);
      }
   } 

mea_write_cfgfile_clean_exit:
   fclose(fd);
   return ret_code;
}


int mea_cfgfile_load(char *file, struct cfgfile_params_s *cfgfile_params)
{
   FILE *fd;

   if(cfgfile_params==NULL || cfgfile_params->nb_params <= 0 || cfgfile_params->keysvalues_list==NULL)
      return -1;
 
   fd=fopen(file, "r");
   if(!fd)
   {
      perror("");
      return -1;
   }

   struct cfgfile_keyvalue_s *keys_names_list = cfgfile_params->keysvalues_list;
 
   int i=0;
   for(;keys_names_list[i].key;i++)
   {
      if(cfgfile_params->keysvalues_list[i].value)
      {
         free(cfgfile_params->keysvalues_list[i].value);
         cfgfile_params->keysvalues_list[i].value=NULL;
      }
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
         // suppression du commentaire éventuellement présent dans la ligne
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
            VERBOSE(9) mea_log_printf("warning, syntax error : %s, line %d not a \"key = value\" line\n", file, i);
            continue;
         }
         
         char *tkey=mea_strtrim(key);
         char *tvalue=mea_strtrim(value);
         mea_strtolower(tkey);
//         mea_strtolower(tvalue);

         struct cfgfile_keyvalue_s *param=_getKeyValue(cfgfile_params, tkey);

         if(param==NULL)
         {
            mea_log_printf("warning : \"%s\" unknown parameter\n",tkey);
            continue;
         }

         if(param->value)
            free(param->value); 
         param->value=(char *)malloc(strlen(tvalue)+1);
         if(param->value==NULL)
         {
            mea_log_printf("error - malloc error\n");
            goto mea_load_config_file_clean_exit;
         }

         strcpy(param->value,tvalue);
      }
   }
 
   return 0;
   
mea_load_config_file_clean_exit:
   for(i=0;i<cfgfile_params->nb_params;i++)
   {
      if(cfgfile_params->keysvalues_list[i].value)
      {
         free(cfgfile_params->keysvalues_list[i].value);
         cfgfile_params->keysvalues_list[i].value=NULL;
      }
   }

   return -1;
}


struct arg_s {
   char *var;
   char *val;
};


int mea_create_file_from_template(char *template_file, char *dest_file, ...)
{
   va_list argp, argp_c;
   struct arg_s *args;
   int args_count=0;

   FILE *fd_in=NULL, *fd_out=NULL;
   struct mea_memfile_s *mf=NULL;
   char line[255];

   int ret_code=0;

   va_start(argp, dest_file);
   va_copy(argp_c, argp);
   char *s;
   while(s=va_arg(argp_c, char *))
      args_count++;

   if((args_count%2)==0)
   {
      args_count=args_count/2;

      args=(struct arg_s *)malloc(sizeof(struct arg_s)*args_count);   

      int i=0;
      for(;i<args_count;i++)
      {
         s=va_arg(argp, char *);
         args[i].var=(char *)malloc(strlen(s)+1);
         strcpy(args[i].var,s);

         s=va_arg(argp, char *);
         args[i].val=(char *)malloc(strlen(s)+1);
         strcpy(args[i].val,s);
      }
   }
   else
      ret_code=0;
   va_end(argp);

   if(ret_code!=0)
      return ret_code;

   fd_in=fopen(template_file, "r");
   if(!fd_in)
   {
      fprintf(stderr,"can't open template file %s : ", template_file);
      perror("");
      goto create_cfg_from_template_clean_exit;
   }

   mf=mea_memfile_init(mea_memfile_alloc(), AUTOEXTEND, 1024, 1);

   int lnum=0;
   while(!feof(fd_in))
   {
      if(fgets(line,sizeof(line)-1,fd_in) != NULL)
      {
         lnum++;
         char var[80]="";
         int i=0;
         for(;line[i];i++)
         {
            if(line[i]=='[' && line[i+1]=='[')
            {
               i+=2;
               int j=0;
               int flag=-1;
               for(;line[i];i++)
               {
                  if(line[i]==']' && line[i+1]==']')
                  {
                     i+=1;
                     var[j]=0;
                     flag=0;
                     break;
                  }
                  else
                  {
                     var[j++]=line[i];
                  }
               }
               if(flag==0)
               {
                  int i=0;
                  int found_flag=-1;
                  for(;i<args_count;i++)
                  {
//                     fprintf(stderr,"'%s'=='%s'?\n",var,args[i].var);
                     if(strcmp(var,args[i].var)==0)
                     {
                        mea_memfile_printf(mf, "%s", args[i].val);
                        found_flag=0;
                        break;
                     }
                  }
                  if(found_flag!=0)
                  {
                     fprintf(stderr,"unknow substitution tag (%s) at line %d\n",var,lnum);
                     goto create_cfg_from_template_clean_exit;
                  }
               }
               else
               {
                  fprintf(stderr,"Template error line %d\n", lnum);
                  goto create_cfg_from_template_clean_exit;
               }
               var[0]=0;
            }
            else
            {
               mea_memfile_printf(mf, "%c", line[i]);
            }
         }
      }
   }
  
   fd_out=fopen(dest_file, "w");
   if(!fd_out)
   {
      fprintf(stderr,"can't create/open %s : ", dest_file);
      perror("");
      goto create_cfg_from_template_clean_exit;
   }

   int i=0;
   for(;mf->data[i];i++) 
   {
      putc(mf->data[i], stderr);
      putc(mf->data[i], fd_out);
   }

   fclose(fd_out);

   ret_code=0;

create_cfg_from_template_clean_exit:
   if(fd_in!=NULL)
      fclose(fd_in);

   if(mf)
   {
      mea_memfile_release(mf);
      free(mf);
      mf=NULL;
   }

   if(args)
   {
      int i=0;
      for(;i<args_count;i++)
      {
         free(args[i].var);
         args[i].var=NULL;
         free(args[i].val);
         args[i].val=NULL;
      }
      free(args);
      args=NULL;
   }

   return ret_code;
}


#ifdef MEA_CFGFILE_MODULE_R7
int main(int argc, char *argv[])
{
   mea_create_file_from_template("../etc/interfaces.template", "interfaces.txt", "boxip", "192.168.10.2", "netmask", "255.255.255.0", NULL);
   fprintf(stderr,"\n");
}
#endif
