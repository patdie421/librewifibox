#ifndef __configure_h
#define __configure_h

#include "tokens.h"
#include "hd44780.h"
#include "i2c_hd44780.h"
#include "i2c_hd44780_pcf8574.h"
#include "lcd.h"

struct lcd_s *lcd_form_cfgfile_alloc(char *cfgfile);

#endif
