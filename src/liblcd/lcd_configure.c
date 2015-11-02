#include <stdio.h>
#include <string.h>

#include "lcd_configure.h"

#include "const.h"
#include "utils.h"
#include "tokens.h"
#include "hd44780.h"
#include "i2c_hd44780.h"
#include "i2c_hd44780_pcf8574.h"
#include "lcd.h"


enum token_id_e {
   _FIRST_PARAM=0,

   INTERFACE_ID=_FIRST_PARAM,
   I2C_PORT_ID,
   I2C_DEVICE_TYPE_ID,
   I2C_DEVICE_ADDR_ID,
   PCF8574_PIN_MAP_ID,
   LCD_TYPE_ID,
   LCD_SCREEN_ID,
   LCD_BLINVERSION_ID,
   _LAST_PARAM,

   _FIRST_VALUE=_LAST_PARAM,

   PCF8574_ID,
   I2C_ID=_FIRST_VALUE,
   HD44780_ID,
};


struct token_s keys_names_list[]={
   {"interface",       INTERFACE_ID},
   {"i2c_port",        I2C_PORT_ID},
   {"i2c_device_type", I2C_DEVICE_TYPE_ID},
   {"i2c_device_addr", I2C_DEVICE_ADDR_ID},
   {"pcf8574_pin_map", PCF8574_PIN_MAP_ID},
   {"lcd_type",        LCD_TYPE_ID},
   {"lcd_screen",      LCD_SCREEN_ID},
   {"backlight_inversion", LCD_BLINVERSION_ID},
   {NULL,0}
};


// liste des interfaces connues (uniquement i2c pour l'instant)
struct token_s interface_list[]={
   {"i2c",       I2C_ID},
   {NULL,0}
};


// liste des "periphéques" i2c connus (uniquement pcf8574 pour l'instant)
struct token_s i2c_device_type_list[]={
   {"pcf8574",   PCF8574_ID},
   {NULL,0}
};


// liste de type de LCD connus pour le moment
struct token_s lcd_type_list[]={
   {"hd44780",   HD44780_ID},
   {NULL,0}
};


// configurations d'ecran lcd connus
struct token_s lcd_screen_list[]={
   {"8x1",    LCD8x1},
   {"16x1T1", LCD16x1T1},
   {"16x1T2", LCD16x1T2},
   {"8x2",    LCD8x2},
   {"16x2",   LCD16x2},
   {"20x2",   LCD20x2},
   {"16x4",   LCD16x4},
   {"20x4",   LCD20x4},
   {NULL,0}
};


static int _load_cfgfile(char *file, char *params[])
{
   FILE *fd;
 
   fd=fopen(file, "r");
   if(!fd)
   {
      perror("");
      return -1;
   }
 
   int i=0;
   for(i=0;i<_LAST_PARAM-_FIRST_PARAM;i++)
   {
      free(params[i]);
      params[i]=NULL;
   }
 
   char line[255];
   char *noncomment[2];
   char key[20], value[40];
   char *tkey,*tvalue;
 
   i=0;
   while(!feof(fd))
   {
      if(fgets(line,sizeof(line),fd) != NULL)
      {
         i++;
         // suppression du commentaire éntuellement présent dans la ligne
         strsplit(line, '#', noncomment, 2);
         char *keyvalue=strtrim(noncomment[0]);
         if(strlen(keyvalue)==0)
            continue;
 
         // lecture d'un couple key/value
         key[0]=0;
         value[0]=0;
         sscanf(keyvalue,"%[^=]=%[^\n]", key, value);
 
         if(key[0]==0 || value[0]==0)
         {
            fprintf(stderr,"%s - syntax error : line %d not a \"key = value\" line\n", i, warning_str);
            goto load_config_file_clean_exit;
         }
         
         char *tkey=strtrim(key);
         char *tvalue=strtrim(value);
         strtolower(tkey);
         strtolower(tvalue);

         int id;
         id=getTokenID(keys_names_list, tkey);
         if(id<0)
         {
            fprintf(stderr,"%s - Unknow parameter\n", warning_str);
            continue;
         }
 
         params[id-_FIRST_PARAM]=(char *)malloc(strlen(tvalue)+1);
         if(params[id-_FIRST_PARAM]==NULL)
         {
            fprintf(stderr,"%s - malloc error\n", warning_str);
            continue;
         }
 
         strcpy(params[id-_FIRST_PARAM],tvalue);
      }
   }
 
   return 0;
   
load_config_file_clean_exit:
   for(i=0;i<_LAST_PARAM-_FIRST_PARAM;i++)
   {
      free(params[i]);
      params[i]=NULL;
   }
   return -1;
}


struct lcd_s *i2c_hd44780_lcd_alloc( char *params[])
{
   int n = -1;

   struct lcd_s *lcd;
   lcd=lcd_alloc();
   if(lcd==NULL)
   {
      fprintf(stderr,"%s - malloc error : ",error_str);
      return NULL;
   }


   int port = -1;
   if(params[I2C_PORT_ID]==NULL)
   {
      fprintf(stderr,"%s - i2c_port is mandatory for i2c interface\n", error_str);
      return NULL;
   }
   int ret=sscanf(params[I2C_PORT_ID],"%d%n",&port,&n);
   if(port < 0 || n != strlen(params[I2C_PORT_ID]))
   {
      fprintf(stderr,"%s - i2c_port must be numeric and positiv\n", error_str);
      return NULL;
   }
#ifdef DEBUG
   fprintf(stderr,"%s - i2c_port = %d\n", debug_str, port);
#endif 

   int addr = -1;
   if(params[I2C_DEVICE_ADDR_ID]==NULL)
   {
      fprintf(stderr,"%s - i2c_device_addr is mandatory for i2c interface\n", error_str);
      return NULL;
   }
   if(params[I2C_DEVICE_ADDR_ID][1]=='x') // decimal ou hexa ?
      sscanf(params[I2C_DEVICE_ADDR_ID],"%x%n",&addr,&n);
   else
      sscanf(params[I2C_DEVICE_ADDR_ID],"%d%n",&addr,&n);
 
   if(addr < 1 || addr > 127 || n != strlen(params[I2C_DEVICE_ADDR_ID]))
   {
      fprintf(stderr,"%s - i2c_device_addr must be in range 1 to 127\n", error_str);
      return NULL;
   }
#ifdef DEBUG
   else
      fprintf(stderr,"%s - i2c_device_addr = %x\n", debug_str, addr);
#endif 

   int lcd_screen = getTokenID(lcd_screen_list, params[LCD_SCREEN_ID]);
   if(lcd_screen<0)
   {
      fprintf(stderr,"%s - unknown lcd type : %s\n", error_str, params[LCD_TYPE_ID]);
      return NULL;
   }

   
   int i2c_device_type_id = getTokenID(i2c_device_type_list, params[I2C_DEVICE_TYPE_ID]);
   if(i2c_device_type_id<0)
   {
      fprintf(stderr,"%s - unknown i2c_device_type : %s\n", error_str, params[I2C_DEVICE_TYPE_ID]);
      return NULL;
   }


   switch(i2c_device_type_id)
   {
      case PCF8574_ID:
      {
         int pcf8574_map[8];
         int i,j;

         if(params[PCF8574_PIN_MAP_ID]==NULL)
         {
            fprintf(stderr,"%s - pcf8574_pin_map is mandatory for i2c/pcf8574 interface/device\n", error_str);
            return NULL;
         }
         sscanf(params[PCF8574_PIN_MAP_ID],"%d,%d,%d,%d,%d,%d,%d,%d%n",&pcf8574_map[0],&pcf8574_map[1],&pcf8574_map[2],&pcf8574_map[3],&pcf8574_map[4],&pcf8574_map[5],&pcf8574_map[6],&pcf8574_map[7],&n);
         if(n != strlen(params[PCF8574_PIN_MAP_ID]))
         {
             fprintf(stderr,"%s - incorrect or incomplet pcf8574 pins mapping\n", error_str);
             return NULL;
         }
         for(i=0;i<8;i++)
         {
            if(pcf8574_map[i]<0 || pcf8574_map[i]>7)
            {
               fprintf(stderr,"%s - incorrect pcf8574 pin value (%d)\n", error_str, pcf8574_map[i]);
               return NULL;
            }

            for(j=i+1;j<8;j++)
            {
               if(pcf8574_map[i]==pcf8574_map[j])
               {
                  fprintf(stderr,"%s - duplicate pcf8574 pin value (%d)\n", error_str, pcf8574_map[i]);
                  lcd_free(&lcd);
                  return NULL;                    
               }
            }
         }

         lcd->iface_context   = i2c_hd44780_iface_context_alloc(port, addr, PCF8574, lcd_screen, pcf8574_map);
         if(lcd->iface_context == NULL)
         {
            lcd_free(&lcd);
            lcd=NULL;
            return NULL;
         }

         lcd->iface_init      = i2c_hd44780_init;
         lcd->iface_close     = i2c_hd44780_close;
         lcd->iface_backlight = i2c_hd44780_backlight;
         lcd->iface_clear     = i2c_hd44780_clear;
         lcd->iface_print     = i2c_hd44780_print;
         lcd->iface_gotoxy    = i2c_hd44780_gotoxy;
         return lcd;
         break;
      }

      default:
         fprintf(stderr,"%s - unknown i2c_device_type : %s\n", error_str, params[I2C_DEVICE_TYPE_ID]);
         return NULL;
   }
}


struct lcd_s *lcd_form_cfgfile_alloc(char *cfgfile)
{
   char *params[_LAST_PARAM - _FIRST_PARAM];
   struct lcd_s *lcd=NULL;
   int ret=0;
   
      // raz params
   int i=0;
   for(i=0;i<_LAST_PARAM - _FIRST_PARAM;i++)
      params[i]=NULL;
 
   ret=_load_cfgfile(cfgfile, params);
   if(ret==-1)
   {
      fprintf(stderr,"%s - can't load interface configuration file (%s) : ",error_str, cfgfile);
      perror("");
      goto lcd_form_config_file_alloc_clean_exit;
   }

#ifdef DEBUG  
   // display all parameters found
   for(i=0;i<_LAST_PARAM;i++)
   {
      if(params[i]!=NULL)
         fprintf(stderr, "%s - %d : %s\n", debug_str, i, params[i]);
      else
         fprintf(stderr, "%s - %d : NULL\n", debug_str, i);
   }
#endif
 
   // get interface
   int interface_id = getTokenID(interface_list, params[INTERFACE_ID]);
   if(interface_id<0)
   {
      fprintf(stderr,"%s - unknown interface : %s\n", error_str, params[INTERFACE_ID]);
      ret=-1;
      goto lcd_form_config_file_alloc_clean_exit;
   }


   // get lcd type
   int lcd_type_id = getTokenID(lcd_type_list, params[LCD_TYPE_ID]);
   if(lcd_type_id<0)
   {
      fprintf(stderr,"%s - unknown lcd type : %s\n", error_str, params[LCD_TYPE_ID]);
      ret=-1;
      goto lcd_form_config_file_alloc_clean_exit;
   }

   int lcd_backlight_inversion=0; // pas d'inversion par défaut
   if(params[LCD_BLINVERSION_ID])
   {
      int n;
      sscanf(params[LCD_BLINVERSION_ID],"%d%n",&lcd_backlight_inversion,&n);
      if(n!=strlen(params[LCD_BLINVERSION_ID]) || lcd_backlight_inversion<0 || lcd_backlight_inversion>1)
      {
         fprintf(stderr,"%s - incorrect backlight inversion value : %s\n", error_str, params[LCD_BLINVERSION_ID]);
         ret=-1;
         goto lcd_form_config_file_alloc_clean_exit;      
      }
   }
 
   void *iface_context = NULL;
   switch(interface_id)
   {
      case I2C_ID:
         switch(lcd_type_id)
         {
            case HD44780_ID:
               lcd=i2c_hd44780_lcd_alloc(params);
               if(lcd==NULL)
               {
                  ret=-1;
                  goto lcd_form_config_file_alloc_clean_exit;
               }
               break;
            default:
               fprintf(stderr,"%s - incompatible interface/lcd_type : %s\n", error_str, params[LCD_TYPE_ID]);
               ret=-1;
               goto lcd_form_config_file_alloc_clean_exit;
               break;
         }
         break;
         
      default:
         ret=-1;
         fprintf(stderr,"%s - unknow interface : %s\n", error_str, params[INTERFACE_ID]);
         goto lcd_form_config_file_alloc_clean_exit;
         break;
   }
 
   if(lcd==NULL)
   {
      ret=-1;
      goto lcd_form_config_file_alloc_clean_exit;
   }
   
   lcd->backlight_inversion = lcd_backlight_inversion;

lcd_form_config_file_alloc_clean_exit:
   for(i=0;i<_LAST_PARAM - _FIRST_PARAM;i++)
   {
      if(params[i]);
      {
         free(params[i]);
         params[i]=NULL;
      }
   }
   
   if(ret==0)
      return lcd;
   else
   {
      if(lcd)
         lcd_free(&lcd);
      return NULL;
   }
}
