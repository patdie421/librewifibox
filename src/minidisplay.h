#ifndef __minidisplay_h
#define __minidisplay_h

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>

#include "lcd.h"

enum driver_type_e { LCD=1 };
struct display_driver_s
{
   enum driver_type_e type;
   union driver_u {
      struct lcd_s *lcd;
   } descriptor;
};


struct mini_display_s
{
   struct display_driver_s driver;
   uint16_t nb_pages;
   uint16_t c,r;

   char *pages_buf;
   char *display_buf; 
   char *diff_buf;
   uint16_t *cursors;

//   uint16_t cursor;
   uint16_t display_page;

   pthread_t *thread;
   pthread_mutex_t *pages_locks;
};

int mini_display_clear_screen(struct mini_display_s *mini_display, uint16_t page);
int mini_display_gotoxy(struct mini_display_s *mini_display, uint16_t page, uint16_t x, uint16_t y);
int mini_display_print(struct mini_display_s *mini_display, uint16_t page, char *str);
int mini_display_release(struct mini_display_s *mini_display);
int mini_display_init(struct mini_display_s *mini_display, int nb_pages, int nb_columns, int nb_rows);
int mini_display_clear_line(struct mini_display_s *mini_display, uint16_t page, uint16_t line);
void mini_display_xy_printf(struct mini_display_s *mini_display, uint16_t page, uint16_t x, uint16_t y, int cl_flag, char const* fmt, ...);
int mini_display_change_page(struct mini_display_s *mini_display, uint16_t page_num);
int mini_display_set_driver(struct mini_display_s *mini_display, enum driver_type_e type, void *driver);

#endif
