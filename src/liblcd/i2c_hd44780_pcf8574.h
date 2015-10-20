#ifndef __i2c_hd44780_pcf8574_h
#define __i2c_hd44780_pcf8574_h

extern char default_pcf8574_pins[];

enum i2c_hd44780_pcf8574_RS_e { DATA = 1, CMND = 0 };

typedef char * i2c_hd44780_pcf8574_pins_map_t;

void i2c_hd44780_pcf8574_set_pins_mapping(i2c_hd44780_pcf8574_pins_map_t pcf8574_pins, char d4, char d5, char d6, char d7, char en, char rs, char rw, char blight);

int i2c_hd44780_pcf8574_init(int fd, i2c_hd44780_pcf8574_pins_map_t pcf8574_pins);

int i2c_hd44780_pcf8574_write_byte(int fd, unsigned char data, enum i2c_hd44780_pcf8574_RS_e rs, int blight, i2c_hd44780_pcf8574_pins_map_t pcf8574_pins);
int i2c_hd44780_pcf8574_backlight(int fd, int blight, i2c_hd44780_pcf8574_pins_map_t pcf8574_pins);

#endif
