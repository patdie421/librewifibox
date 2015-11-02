#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

#include "lcd.h"


struct lcd_s *lcd_alloc()
/**
 * \brief     allocation (ie malloc) d'une structure lcd_s.
 * \return    pointeur sur une structure de type lcd_s ou NULL en cas d'erreur d'allocation
 */
{
   struct lcd_s *lcd = (struct lcd_s *)malloc(sizeof(struct lcd_s));
   if(lcd)
   {
      lcd->iface_context=NULL;
      lcd->iface_init=NULL;
      lcd->iface_close=NULL;
      lcd->iface_backlight=NULL;
      lcd->iface_clear=NULL;
      lcd->iface_print=NULL;
      lcd->iface_gotoxy=NULL;
   
      lcd->nb_rows=-1;
      lcd->nb_columns=-1;
   
      lcd->backlight_inversion=0;
   
      lcd->x=0;
      lcd->y=0;
   }
   
   return lcd;
}


void lcd_free(struct lcd_s **lcd)
/**
 * \brief     libère une la structure de type lcd_s.
 * \details   cette fonction libère aussi la zone mémoire contenant le context de l'interface
 * \param     lcd    pointeur sur le pointeur de la structure lcd_s.
 */
{
   if((*lcd)->iface_context)
   {
      free((*lcd)->iface_context);
      (*lcd)->iface_context=NULL;
   }
   free(*lcd);
   *lcd=NULL;
}


int lcd_init(struct lcd_s *lcd, int reset)
/**
 * \brief     initialise une structure lcd_s et si demandé (reset = 1) initialise la communication avec le contrôleur LCD.
 * \details   attention, avant de pouvoir utiliser cette fonction il faut que le "driver" lcd ait été associé à lcd.
 * \param     lcd    pointeur sur la structure lcd_s.
 * \param     reset  drapeau pour le déclenchement de l'initialisation du contrôleur LCD
 * \param     speed  debit du pour serie
 * \return    0 OK, -1 en cas d'erreur
 */
{
   lcd->x=0;
   lcd->y=0;

   if(lcd->iface_init)
      return lcd->iface_init(lcd->iface_context, reset);
   else
      return -1;
}


void lcd_backlight_inversion(struct lcd_s *lcd, int inv)
/**
 * \brief     permet de positionner l'indicateur d'inversion du rétro-éclairage.
 * \details   pour certain afficheur la commande de rétroéclairage est inversée : ON => LED éteinte, OFF => LED allumée. Dans ce cas il faut positionné inv à 1.
 * \param     lcd    pointeur sur la structure lcd_s.
 * \param     inv    0 = pas d'inversion, 1 = inversion
 */
{
   if(inv > 0)
      lcd->backlight_inversion=1;
   else
      lcd->backlight_inversion=0;
}


int lcd_release(struct lcd_s *lcd)
/**
 * \brief     finalise la communication avec un contrôleur LCD.
 * \details   appel la fonction "iface_close" du driver de l'écran.
 * \param     lcd    pointeur sur la structure lcd_s.
 * \return    0 OK, -1 en cas d'erreur
 */
{
   if(lcd->iface_close)
      return lcd->iface_close(lcd->iface_context);
   else
      return -1;
}


int lcd_backlight(struct lcd_s *lcd, enum backlight_state_e state)
{
/**
 * \brief     change l'état du rétro-éclairage.
 * \param     lcd    pointeur sur la structure lcd_s.
 * \param     state  nouvel état du rétro-éclairage (ON / OFF)
 * \return    0 OK, -1 en cas d'erreur
 */
   if(lcd->backlight_inversion == 1)
   {
      if(state==ON)
         state=OFF;
      else
         state=ON;
   }

   if(lcd->iface_backlight)
      return lcd->iface_backlight(lcd->iface_context, state);
   else
      return -1;
}


int lcd_clear(struct lcd_s *lcd)
/**
 * \brief     efface l'écran LCD et positionne le curseur en (0,0).
 * \details   positionne x et y à 0 et appel la fonction "iface_clear" du driver de l'écran.
 * \param     lcd    pointeur sur la structure lcd_s.
 * \return    0 OK, -1 en cas d'erreur
 */
{
   lcd->x=0;
   lcd->y=0;
   if(lcd->iface_clear)
      return lcd->iface_clear(lcd->iface_context);
   else
      return -1;
}


int lcd_gotoxy(struct lcd_s *lcd, uint16_t x, uint16_t y)
/**
 * \brief     positionne le curseur en (x,y).
 * \param     lcd    pointeur sur la structure lcd_s.
 * \param     x      nouvelle position "x" du curseur
 * \param     y      nouvelle position "y" du curseur
 * \return    0 OK, -1 en cas d'erreur
 */
{
   if(!lcd->iface_gotoxy)
      return -1;
      
   lcd->x=x;
   lcd->y=y;

   return 0;
//   return lcd->iface_gotoxy(lcd->iface_context, lcd->x, lcd->y);
}

   
int lcd_print(struct lcd_s *lcd, char *str)
/**
 * \brief     affiche une chaine sur l'écran LCD.
 * \param     lcd    pointeur sur la structure lcd_s.
 * \param     str    chaine à afficher
 * \return    0 OK, -1 en cas d'erreur
 */
{
   if(lcd->iface_print)
      return lcd->iface_print(lcd->iface_context, &(lcd->x), &(lcd->y), str);
   else
      return -1;
}


int lcd_printf(struct lcd_s *lcd, char const* fmt, ...)
/**
 * \brief     affiche une chaine "formatée" sur LCD.
 * \details   même syntaxe que printf.
 * \param     lcd    pointeur sur la structure lcd_s.
 * \param     fmt    format de la chaine à afficher (voir printf)
 * \param     ...    liste des variables à formater (voir printf)
 * \return    0 OK, -1 en cas d'erreur
 */
{
   char *str=(char *)malloc(lcd->nb_rows * lcd->nb_columns);
   if(!str)
      return -1;
      
   va_list args;

   va_start(args, fmt);
   vsnprintf(str, lcd->nb_rows * lcd->nb_columns, fmt, args);
   va_end(args);

   int ret=lcd_print(lcd, str);
   
   free(str);
   
   return ret;
}
