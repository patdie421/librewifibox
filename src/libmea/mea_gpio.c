#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

#include "mea_gpio.h"

#include "mea_verbose.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 32
#define POLL_TIMEOUT (1000) /* 1 seconds */

//
// pour pouvoir utiliser le gpio sans être root la règle si dessous
// peut être rajoutée dans /etc/udev/rules.d/99-gpio.rules :
//
// SUBSYSTEM=="gpio*", PROGRAM="/bin/sh -c 'chown -R root:i2c /sys/class/gpio; chmod -R 770 /sys/class/gpio; chown -R root:i2c /sys/devices/virtual/gpio; chmod -R 770 /sys/devices/virtual/gpio'"
//
// lancer la commande « sudo udevadm test --action=add /class/gpio » après mise à jour du fichier
//

int mea_gpio_export(unsigned int gpio)
{
   int fd, len;
   char buf[MAX_BUF];

   fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
   if (fd < 0)
      return fd;

   len = snprintf(buf, sizeof(buf), "%d", gpio);
   write(fd, buf, len);

   close(fd);

   return 0;
}


int mea_gpio_unexport(unsigned int gpio)
{
   int fd, len;
   char buf[MAX_BUF];

   fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);

   if (fd < 0)
      return fd;

   len = snprintf(buf, sizeof(buf), "%d", gpio);
   write(fd, buf, len);

   close(fd);
   return 0;
}


int mea_gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
   int fd, len;
   char buf[MAX_BUF];

   len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
   fd = open(buf, O_WRONLY);
   if (fd < 0)
      return fd;

   if (out_flag)
      write(fd, "out", 4);
   else
      write(fd, "in", 3);

   close(fd);

   return 0;
}

// valeur possible pour edge : rising/falling/both/none
int mea_gpio_set_edge(unsigned int gpio, char *edge)
{
   int fd, len;
   char buf[MAX_BUF];

   len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

   fd = open(buf, O_WRONLY);
   if (fd < 0)
      return fd;

   write(fd, edge, strlen(edge) + 1);

   close(fd);

   return 0;
}


int mea_gpio_fd_open(unsigned int gpio)
{
   int fd, len;
   char buf[MAX_BUF];

   len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

   fd = open(buf, O_RDONLY | O_NONBLOCK );

   return fd;
}


int mea_gpio_fd_close(int fd)
{
   if(fd<0)
      return -1;

   return close(fd);
}

  
int mea_gpio_wait_intr(int fd, int timeout_ms)
{
   struct pollfd fdset[2];
   int nfds = 2;
   int gpio_fd, rc;
   char *buf[MAX_BUF];
   int len;

   if(fd<0)
      return -1;

   // flush 

   memset((void*)fdset, 0, sizeof(fdset));
   fdset[0].fd = fd;
   fdset[0].events = POLLPRI;

   while(read(fd,buf,sizeof(buf)));

   rc = poll(fdset, nfds, timeout_ms);
   if (rc < 0)
      return -1;

   if (rc == 0)
      return 0; // timeout

   // reception d'une interruption GPIO

   if (fdset[0].revents & POLLPRI)
   {
      len = read(fdset[0].fd, buf, MAX_BUF);
      DEBUG_SECTION mea_log_printf("interruption recept (buff len = %d)",len);
      return 1;
   }
}     

/*
int mea_gpio_init_falling_intr(int gpio_pin)
{
   int fd;

   mea_gpio_set_dir(gpio_pin, 0);
   mea_gpio_set_edge(gpio_pin, "falling");

   fd = mea_gpio_fd_open(gpio_pin);

   return fd;
}
*/

int mea_gpio_get_state(int fd)
{
   char buf[8];
   int state=-1;

   if(fd<0)
      return -1;

   if(lseek(fd, 0, SEEK_SET)!=-1)
   {
      int nb=read(fd, buf, sizeof(buf));
      if(nb>0)
      {
         buf[nb]=0;
         state=buf[0]-'0';
         return state;
      }
      else
         return -1;
   }
   else
      return -1;
}
