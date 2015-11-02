#ifndef __mea_gpio_h
#define __mea_gpio_h

int mea_gpio_export(unsigned int gpio);
int mea_gpio_unexport(unsigned int gpio);
int mea_gpio_set_dir(unsigned int gpio, unsigned int out_flag);
int mea_gpio_set_edge(unsigned int gpio, char *edge);
int mea_gpio_fd_open(unsigned int gpio);
int mea_gpio_fd_close(int fd);
int mea_gpio_wait_intr(int fd, int timeout_ms);
//int mea_gpio_init_falling_intr(int gpio_pin);
int mea_gpio_get_state(int fd);

#endif
