#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <inttypes.h>
#include <string.h>

#include "const.h"
#include "utils.h"

#include "i2c.h"
#include "hd44780.h"
#include "i2c_hd44780.h"
#include "i2c_hd44780_pcf8574.h"

struct i2c_hd44780_iface_context_s *i2c_hd44780_iface_context_alloc(int port, int device, enum device_type_e device_type, enum hd44780_type_e hd44780_type, int *pins_map)
/**
 * \brief     allocation et initialisation des données de context du driver.
 * \param     port         numéro du port i2c.
 * \param     device_type  référence du composant i2c qui pilotera l'écran hd44780 (ex : PCF8574)
 * \param     hd44780_type type (ie taille) d'écran HD44780 (ex LCD8x1, voir hd44789.h pour les écrans supportés)
 * \param     pins_map     mapping (association des connexion HD44780 et du composant "pilote" i2c), NULL pas utils au pilote i2c
 * \return    structure allouée ou NULL en cas d'erreur
 */
{
   struct i2c_hd44780_iface_context_s *iface_context = (struct i2c_hd44780_iface_context_s *)malloc(sizeof(struct i2c_hd44780_iface_context_s));

   if(!iface_context)
      return NULL;

   iface_context->fd=-1;
   iface_context->port=port;
   iface_context->device_type=device_type;
   iface_context->device=device;
   iface_context->hd44780_type=hd44780_type;
   iface_context->backlight=0;
   iface_context->backlight_inversion=0;

   int i=0;
   if(pins_map)
   {
      for(i=0;i<8;i++)
         iface_context->pins_map[i]=pins_map[i];
   }
   
   switch(iface_context->device_type)
   {
      case PCF8574:
         iface_context->i2c_device_drivers.device_init       = i2c_hd44780_pcf8574_init;
         iface_context->i2c_device_drivers.device_write_byte = i2c_hd44780_pcf8574_write_byte;
         iface_context->i2c_device_drivers.device_backlight  = i2c_hd44780_pcf8574_backlight;
         break;
      default:
         return NULL;
   }

   return iface_context;
}


void i2c_hd44780_iface_context_free(struct i2c_hd44780_iface_context_s **context)
/**
 * \brief     libère le context d'un driver.
 * \param     context  référence du composant i2c qui pilotera l'écran hd44780 (ex : PCF8574)
 */
{
   free(*context);
   *context=NULL;
}


int i2c_hd44780_backlight(void *iface_context, int blight)
{
/**
 * \brief     change l'état du rétro-éclairage.
 * \param     iface_context  context du driver.
 * \param     blight  état du rétro-éclairage (ON / OFF)
 * \return    0 OK, -1 en cas d'erreur
 */
   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   if(!context || !context->i2c_device_drivers.device_backlight)
      return -1;

   context->backlight=blight;

   context->i2c_device_drivers.device_backlight(context->fd, context->backlight, context->pins_map);

   return 0;
}


static int _i2c_hd44780_write_data(void *iface_context, char data)
/**
 * \brief     envoi 1 caractère à afficher au contrôleur.
 * \param     iface_context  context du driver.
 * \param     data           charactère à afficher
 * \return    0 OK, -1 en cas d'erreur
 */
{
   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   if(!context || !context->i2c_device_drivers.device_write_byte)
      return -1;

   context->i2c_device_drivers.device_write_byte(context->fd, data, DATA, context->backlight, context->pins_map);

   return 0;
}


static int _i2c_hd44780_write_cmnd(void *iface_context, char data)
/**
 * \brief     envoi 1 commande à traiter par le contrôleur contrôleur.
 * \param     iface_context  context du driver.
 * \param     data           charactère à afficher
 * \return    0 OK, -1 en cas d'erreur
 */
{
   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   if(!context || !context->i2c_device_drivers.device_write_byte)
      return -1;

   context->i2c_device_drivers.device_write_byte(context->fd, data, CMND, context->backlight, context->pins_map);

   mymicrosleep(50);
            
   return 0;
}


int i2c_hd44780_init(void *iface_context, int reset)
{
/**
 * \brief     initialise la communication avec un écran LCD.
 * \param     iface_context  context du driver.
 * \param     reset          déclanchement ou non de la séquence d'initialisation de l'écran après établissement de la communication
 * \return    0 OK, -1 en cas d'erreur
 */
   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   if(!context || !context->i2c_device_drivers.device_init)
      return -1;

   context->fd=i2c_open(context->port);

   if(context->fd<0)
      return -1;

   if(i2c_select_slave(context->fd, context->device)<0)
      return -1;

   if(reset)
      return context->i2c_device_drivers.device_init(context->fd, context->pins_map);

   return 0;
}


int i2c_hd44780_close(void *iface_context)
/**
 * \brief     termine la communication avec un écran LCD.
 * \param     iface_context  context du driver.
 * \return    0 OK, -1 en cas d'erreur
 */
{
   if(!iface_context)
      return -1;

   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   return i2c_close(context->fd);
}


int i2c_hd44780_gotoxy(void *iface_context, uint16_t x, uint16_t y)
/**
 * \brief     positionne le curseur en (x, y).
 * \param     iface_context  context du driver.
 * \param     x              nouvelle position "x" du curseur
 * \param     y              nouvelle position "y" du curseur
 * \return    0 OK, -1 en cas d'erreur
 */
{
   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   if(!context)
      return -1;

   int addr=hd44780_get_char_addr(context->hd44780_type, x, y);

   if(addr<0)
      return -1;

   char cmnd=(char)(0b10000000 | (addr & 0b01111111));

   int ret=_i2c_hd44780_write_cmnd(iface_context, cmnd);

   return ret;
}


int i2c_hd44780_clear(void *iface_context)
/**
 * \brief     efface l'écran LCD.
 * \param     iface_context  context du driver.
 * \return    0 OK, -1 en cas d'erreur
 */
{
   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   int ret=_i2c_hd44780_write_cmnd(iface_context, HD44780_CMD_CLEAR_SCREEN);

   mymicrosleep(1500); // nécessaire car l'effacement de l'écran nécessite du temps

   return ret;
}


int i2c_hd44780_print(void *iface_context, uint16_t *x, uint16_t *y, char *str)
/**
 * \brief     affiche une chaine sur l'écran LCD.
 * \details   tous les caractères de la chaine ne sont pas forcement affiché. Si la chaine est trop long pour l'affichage, l'affichage est tronqué.
 * \param     iface_context  context du driver.
 * \param     x,y    position du premier caractère à afficher. En retour, x et y contiennent la position du curseur après affichage
 * \param     str    chaine à afficher.
 * \return    0 OK, -1 en cas d'erreur
 */
{
   struct i2c_hd44780_iface_context_s *context=(struct i2c_hd44780_iface_context_s *)iface_context;

   if(!context)
      return -1;
   int i;
   int set_addr_flag=1; // au premier passage on calcul et position l'adresse du caractère à écrire
   for(i=0;str[i];i++)
   {
      if(str[i]=='\n')
      {
         *y=*y+1;
         *x=0;
         set_addr_flag=1; // au prochain coup on devra recalculer et positionner l'adresse du caractère à écrire
         continue;
      }

      if(str[i]<0b0001000 || str[i]>0b01111010) // caractère affichable ?
         continue;

      if(set_addr_flag==1) // calcul de l'adresse nécessaire
      {
         int addr=hd44780_get_char_addr(context->hd44780_type, *x, *y);
         if(addr<0)
            return -1;
         char cmnd=(char)(0b10000000 | (addr & 0b01111111));
         _i2c_hd44780_write_cmnd(context, cmnd);
      }

      int ret=_i2c_hd44780_write_data(context, str[i]);
      if(ret<0)
         return -1;

      ret=hd44780_next_xy(context->hd44780_type, x, y);
      if(ret<0)
         return -1;
      else if(ret==1) // changement de ligne au prochain tour ?
         set_addr_flag=1;
      else
         set_addr_flag=0;
   }

   return 0;
}
