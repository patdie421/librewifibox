#ifndef __lcd_h
#define __lcd_h

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


struct lcd_s
{
   void *iface_context; // context du driver de l'interface
   
   // points d'entrée du driver de l'inteface
   int (*iface_init)(void *iface_context, int reset);
   int (*iface_close)(void *iface_context);
   int (*iface_backlight)(void *iface_context, int blight);
   int (*iface_clear)(void *iface_context);
   int (*iface_print)(void *iface_context, uint16_t *x, uint16_t *y, char *str);
   int (*iface_gotoxy)(void *iface_context, uint16_t x, uint16_t y);
   
   // taille de l'écran
   uint16_t nb_rows, nb_columns;
   
   // indicateur d'inversion
   uint16_t backlight_inversion;
   
   // position x et y du prochain caractère à afficher
   uint16_t x,y;
};

enum backlight_state_e { ON=1, OFF=0 };

struct lcd_s *lcd_alloc();

void lcd_free(struct lcd_s **lcd);
int  lcd_init(struct lcd_s *lcd, int reset);
int  lcd_release(struct lcd_s *lcd);

int  lcd_backlight(struct lcd_s *lcd, enum backlight_state_e state);
void lcd_backlight_inversion(struct lcd_s *lcd, int inv);
int  lcd_clear(struct lcd_s *lcd);
int  lcd_gotoxy(struct lcd_s *lcd, uint16_t x, uint16_t y);
int  lcd_print(struct lcd_s *lcd, char *str);
int  lcd_printf(struct lcd_s *lcd, char const* fmt, ...);

struct lcd_s *lcd_form_cfgfile_alloc(char *cfgfile);

#endif
