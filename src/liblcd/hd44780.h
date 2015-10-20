#ifndef __hd44780_h
#define __hd44780_h

#include <inttypes.h>

// commande HD44780
#define HD44780_CMD_SET_INTERFACE 0x28 // 4 bits, 2 lignes
#define HD44780_CMD_EN_DISP       0x0A
#define HD44780_CMD_CLEAR_HOME    0x01
#define HD44780_CMD_HOME          0x02
#define HD44780_CMD_SET_CSR       0x06
#define HD44780_CMD_TURN_ON_DISP  0x0C
#define HD44780_CMD_CLEAR_SCREEN  0x01


enum hd44780_type_e { LCD8x1=0, LCD16x1T1=1, LCD16x1T2=2, LCD8x2=3, LCD16x2=4, LCD20x2=5, LCD40x2=6, LCD16x4=7, LCD20x4=8, LCDEND };

int hd44780_get_char_addr(enum hd44780_type_e type, uint16_t x, uint16_t y);
int hd44780_next_xy(enum hd44780_type_e type, uint16_t *x, uint16_t *y);

#endif
