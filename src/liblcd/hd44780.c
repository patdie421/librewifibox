#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "hd44780.h"

#include "utils.h"

static int _hd44780_lines_addrs_1[]={0x00, 0x40, 0x14, 0x54}; // 20x4
static int _hd44780_lines_addrs_2[]={0x00, 0x40, 0x10, 0x50}; // 16x4


struct hd44780_colonnes_lines
{
  uint16_t c;
  uint16_t l;
};


struct hd44780_colonnes_lines _cl_type[]={{8,1}, {16,1}, {16,1}, {8,2}, {16,2}, {20,2}, {40,2}, {16,4}, {20,4}};


int hd44780_get_char_addr(enum hd44780_type_e type, uint16_t x, uint16_t y)
/**
 * \brief     calcul d'adrese du caractère dans le hd44780 à partir de la position x,y en tenant compte du type d'écran.
 * \param     type  d'écran
 * \param     x,y   position souhaité du caractère
 * \return    adresse du caractère, -1 en cas d'erreur (x et y incompatible avec le type d'écran)
 */
{
   int *hd44780_lines_addr;
   
   if (type >= LCDEND)
      return -1;
   
   // selection du mapping mémoire en fonction du type de hd44780
   if (type!=LCD16x4)
      hd44780_lines_addr=_hd44780_lines_addrs_1;
   else
      hd44780_lines_addr=_hd44780_lines_addrs_2;
   
   int addr=0;
   if (x < _cl_type[type].c && y < _cl_type[type].l)
   {
      if(type!=LCD16x1T1)
      {
         addr=hd44780_lines_addr[y]+x;
         return addr;
      }
      else // cas particulier du 16x1 de type 1 (1 ligne géré comme 2 lignes)
      {
         if(x<8)
            addr=hd44780_lines_addr[0]+x;
         else
            addr=hd44780_lines_addr[1]+(x-8);
      }
   }
   else
      return -1;
}


int hd44780_next_xy(enum hd44780_type_e type, uint16_t *x, uint16_t *y)
/**
 * \brief     retourne les nouvelles valeurs de x et y si on ajoute un nouveau caractère.
 * \param     type  d'écran
 * \param     x,y   position souhaité du caractère
 * \return    O RAS, 1 un saut de ligne a été nécessaire, -1 les nouvelles valeurs de x et y sont incompatibles avec le type d'écran
 */
{
   int ret=0;
   if (type < 0 || type >= LCDEND)
      return -1;
      
   *x=*x+1;
   if(*x >= _cl_type[type].c)
   {
      *x=0;
      ret=1; // on signal qu'on a fait un saut de ligne
      *y=*y+1;
      if(*y>=_cl_type[type].l)
         ret=-1;
   }
   
   return ret;
}
