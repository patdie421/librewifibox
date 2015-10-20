#ifndef __i2c_h
#define __i2c_h

#include <inttypes.h>

int i2c_open(int16_t num_port_i2c);
int i2c_write_byte(int fd, char c);
int i2c_write_buffer(int fd, char *buffer, uint16_t l_buffer);
int i2c_close(int fd);
int i2c_select_slave(int fd, int16_t slave);

#endif
