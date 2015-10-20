#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <inttypes.h>

#include "i2c.h"


int i2c_open(int16_t num_port_i2c)
{
   int fd;
   char dev[20];
  
   snprintf(dev, 19, "/dev/i2c-%d", num_port_i2c);
   fd = open(dev, O_RDWR);
   if (fd < 0)
      return -1;

   return fd;
}


int i2c_write_byte(int fd, char c)
{
   return write(fd,&c,1);
}


int i2c_write_buffer(int fd, char *buffer, uint16_t l_buffer)
{
   return write(fd,buffer,l_buffer);
}


int i2c_close(int fd)
{
   close(fd);
}


int i2c_select_slave(int fd, int16_t slave)
{
   if (ioctl(fd, I2C_SLAVE, slave) < 0)
      return -1;
   else
      return 0;
}
