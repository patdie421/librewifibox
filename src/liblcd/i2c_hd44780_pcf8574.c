#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <inttypes.h>


#include "const.h"
#include "utils.h"
#include "i2c.h"
#include "hd44780.h"
#include "i2c_hd44780_pcf8574.h"

#define _HD44780_DATA7  7
#define _HD44780_DATA6  6
#define _HD44780_DATA5  5
#define _HD44780_DATA4  4
#define _HD44780_ENABLE 3
#define _HD44780_RS     2
#define _HD44780_RW     1
#define _HD44780_BLIGHT 0

//char default_i2c_hd44780_pcf8574_pins_map[8]={_HD44780_BLIGHT, _HD44780_RW, _HD44780_RS, _HD44780_ENABLE, _HD44780_DATA4, _HD44780_DATA5, _HD44780_DATA6, _HD44780_DATA7};
char default_i2c_hd44780_pcf8574_pins_map[8]={3, 1, 0, 2, 4, 5, 6, 7};

/*
#define HD44780_DATA7  pcf8574_pins[7]
#define HD44780_DATA6  pcf8574_pins[6]
#define HD44780_DATA5  pcf8574_pins[5]
#define HD44780_DATA4  pcf8574_pins[4]
#define HD44780_ENABLE pcf8574_pins[3]
#define HD44780_RS     pcf8574_pins[2]
#define HD44780_RW     pcf8574_pins[1]
#define HD44780_BLIGHT pcf8574_pins[0]
*/

void i2c_hd44780_pcf8574_set_pins_mapping(i2c_hd44780_pcf8574_pins_map_t pcf8574_pins, char d4, char d5, char d6, char d7, char en, char rs, char rw, char blight)
{
   pcf8574_pins[_HD44780_DATA7]=d7;
   pcf8574_pins[_HD44780_DATA6]=d6;
   pcf8574_pins[_HD44780_DATA5]=d5;
   pcf8574_pins[_HD44780_DATA4]=d4;
   pcf8574_pins[_HD44780_ENABLE]=en;
   pcf8574_pins[_HD44780_RS]=rs;
   pcf8574_pins[_HD44780_RW]=rw;   
   pcf8574_pins[_HD44780_BLIGHT]=blight;
}


static void _i2c_hd44780_pcf8574_lowlevel_write(int fd, unsigned char r, i2c_hd44780_pcf8574_pins_map_t pcf8574_pins)
/**
 * \brief     envoi un octet au hd44780 en respectant son timing bas niveau.
 * \param     fd            descripteur de communication avec le pcf8574.
 * \param     r             octet à envoyer au hd44780
 * \param     pcf8574_pins  mapping hd44780/PCF8574
 */
{
   unsigned char byte = r;
   BITCLEAR(byte, pcf8574_pins[_HD44780_ENABLE]);
   i2c_write_byte(fd, byte);
   mynanosleep(40);

   BITSET(byte, pcf8574_pins[_HD44780_ENABLE]);
   i2c_write_byte(fd, byte);
   mynanosleep(230);

   BITCLEAR(byte, pcf8574_pins[_HD44780_ENABLE]);
   i2c_write_byte(fd, byte);
   mynanosleep(270);
}


int i2c_hd44780_pcf8574_write_byte(int fd, unsigned char data, enum i2c_hd44780_pcf8574_RS_e rs, int blight, i2c_hd44780_pcf8574_pins_map_t pcf8574_pins)
/**
 * \brief     envoi un octet "applicatif" au hd44780.
 * \details   nous sommes en mode 4bits avec un pcf8574. Nous envoyons donc 2 octets (dont seulement les 4bits de poid fort sont significatif) au hd44780
 * \param     fd            descripteur de communication avec le pcf8574.
 * \param     r             octet à envoyer au hd44780
 * \param     pcf8574_pins  mapping hd44780/PCF8574
 */
{
   unsigned char d = 0;

   if(blight)
       BITSET(d, pcf8574_pins[_HD44780_BLIGHT]);
   else
       BITCLEAR(d, pcf8574_pins[_HD44780_BLIGHT]);

   if(rs)
       BITSET(d, pcf8574_pins[_HD44780_RS]);
   else
       BITCLEAR(d, pcf8574_pins[_HD44780_RS]);
   
   // envoie des 4 bits de poid fort
   BITCPY(data, 4, d, pcf8574_pins[_HD44780_DATA4]);
   BITCPY(data, 5, d, pcf8574_pins[_HD44780_DATA5]);
   BITCPY(data, 6, d, pcf8574_pins[_HD44780_DATA6]);
   BITCPY(data, 7, d, pcf8574_pins[_HD44780_DATA7]);

   _i2c_hd44780_pcf8574_lowlevel_write(fd, d, pcf8574_pins);

   // envoie des 4 bits de poid faible   
   BITCPY(data, 0, d, pcf8574_pins[_HD44780_DATA4]);
   BITCPY(data, 1, d, pcf8574_pins[_HD44780_DATA5]);
   BITCPY(data, 2, d, pcf8574_pins[_HD44780_DATA6]);
   BITCPY(data, 3, d, pcf8574_pins[_HD44780_DATA7]);

   _i2c_hd44780_pcf8574_lowlevel_write(fd, d, pcf8574_pins);
}


int i2c_hd44780_pcf8574_init(int fd, i2c_hd44780_pcf8574_pins_map_t pcf8574_pins)
/**
 * \brief     initialisation pcf8574 et hd44780.
 * \details   
 
 Sequence d'initialisation d'un hd44780 :
   - Attendre au moins 15ms depuis le passage a 5V de Vcc,
   - Envoyer l'instruction "Function set" avec la valeur 0011XXXX,
   - Attendre au moins 4.1ms,
   - Envoyer à nouveau l'instruction "Function set" avec la valeur 0011XXXX,
   - Attendre au moins 100 microsecondes,
   - Envoyer à nouveau l'instruction "Function set" avec la valeur 0011XXXX,
   - Si on veut activer le mode 4 bits, envoyer les 4 bits de poids fort de l'instruction
     "Function set" avec la valeur 0010,
   - Configuration du nombre de lignes et de la matrice, en envoyant l'Instruction "Function set"
     avec par exemple la valeur 00111000 (8 bits, 2 lignes, 5x8pixels),
   - Configuration du controle d'affichage, en envoyant l'instruction "Display on/off control" avec par exemple
     la valeur 00001110 (Affichage visible, curseur visible, curseur fixe), // HD44780_CMD_TURN_ON_DISP
   - Effacement de l'ecran, en envoyant l'instruction "Clear display", avec pour valeur 00000001, // HD44780_CMD_CLEAR_HOME
   - Configuration du curseur, en envoyant l'instruction "Entry set mode", avec par exemple pour
     valeur 00000110 (deplacement du curseur vers la droite, pas de decalage du compteur d'adresse). // HD44780_CMD_SET_CSR
   - Fin de l'initialisation, l'ecran est prêt à recevoir les autres instructions permettant l'affichage.
   
 * \param     fd            descripteur de communication avec le pcf8574.
 * \param     pcf8574_pins  mapping hd44780/PCF8574
 */
{
   unsigned char d;
   
   d=0;

   mynanosleep(15 * 1000000); // 15 ms

   BITSET(d, pcf8574_pins[_HD44780_DATA5]);
   BITSET(d, pcf8574_pins[_HD44780_DATA4]);

   mymicrosleep(15000);
   
   // initialisation
   _i2c_hd44780_pcf8574_lowlevel_write(fd, d, pcf8574_pins); // 1er 0x3
   mymicrosleep(5000);

   _i2c_hd44780_pcf8574_lowlevel_write(fd, d, pcf8574_pins); // 2eme 0x3
   mymicrosleep(100);
   
   _i2c_hd44780_pcf8574_lowlevel_write(fd, d, pcf8574_pins); // 4eme 0x3
   mymicrosleep(150);
   
   // passage en mode 4 bits
   BITCLEAR(d, pcf8574_pins[_HD44780_DATA4]);
   _i2c_hd44780_pcf8574_lowlevel_write(fd, d, pcf8574_pins);
   mymicrosleep(1000);

   // préparation divers
   i2c_hd44780_pcf8574_write_byte(fd, HD44780_CMD_SET_INTERFACE, CMND, 1, pcf8574_pins);
   mymicrosleep(50);
   i2c_hd44780_pcf8574_write_byte(fd, HD44780_CMD_EN_DISP, CMND, 1, pcf8574_pins);
   mymicrosleep(50);
   i2c_hd44780_pcf8574_write_byte(fd, HD44780_CMD_CLEAR_HOME, CMND, 1, pcf8574_pins);
   mymicrosleep(1500);
//   mymicrosleep(2000);
   i2c_hd44780_pcf8574_write_byte(fd, HD44780_CMD_SET_CSR, CMND, 1, pcf8574_pins);
   mymicrosleep(50);
   i2c_hd44780_pcf8574_write_byte(fd, HD44780_CMD_TURN_ON_DISP, CMND, 1, pcf8574_pins);
   mymicrosleep(50);

   return 0;
}


int i2c_hd44780_pcf8574_backlight(int fd, int blight, i2c_hd44780_pcf8574_pins_map_t pcf8574_pins)
/**
 * \brief     allume ou éteint le rétro-éclairage.
 * \param     fd            descripteur de communication avec le pcf8574.
 * \param     blight        etat du rétro-éclairage.
 * \param     pcf8574_pins  mapping hd44780/PCF8574.
 */
{
   unsigned char cmnd = 0;
   if(blight)
      BITSET(cmnd, pcf8574_pins[_HD44780_BLIGHT]);
   else
      BITCLEAR(cmnd, pcf8574_pins[_HD44780_BLIGHT]);

   i2c_write_byte(fd, cmnd);

   return 0;
}
