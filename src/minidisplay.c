#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>

#include "minidisplay.h"

#include "lcd.h"

#include "mea_verbose.h"


static inline int _mini_display_commit_diff(struct mini_display_s *mini_display)
{
   char *page_buf=mini_display->pages_buf + (mini_display->display_page*mini_display->c*mini_display->r);

   memcpy(mini_display->display_buf, page_buf, mini_display->c*mini_display->r);

   return 0;
}


static inline int _mini_display_collect_diff(struct mini_display_s *mini_display)
{
   int ret=-1;
   char *page_buf=mini_display->pages_buf + (mini_display->display_page*mini_display->c*mini_display->r);
   int l=mini_display->c*mini_display->r;
   int i;
   for(i=0;i<l;i++)
   {
      if(page_buf[i]!=mini_display->display_buf[i])
      {
         mini_display->diff_buf[i]=page_buf[i];
         ret=0;
      }
      else
         mini_display->diff_buf[i]=0;
   }
   return ret;
}


static inline int _mini_display_send_diff(struct mini_display_s *mini_display)
{
   int l=mini_display->c*mini_display->r;
   int i;

   for(i=0;i<l;i++)
   {
      if(mini_display->diff_buf[i])
      {
         int x,y;
         x=i%mini_display->c;
         y=i/mini_display->c;
         DEBUG_SECTION mea_log_printf("%d,%d : \"%s\"\n",x,y,&(mini_display->diff_buf[i]));
         switch(mini_display->driver.type)
         {
            case LCD:
               lcd_gotoxy(mini_display->driver.descriptor.lcd, x, y);
               lcd_print(mini_display->driver.descriptor.lcd, &(mini_display->diff_buf[i]));
               break;
            default:
               mea_log_printf("NO LCD\n");
               break;
         }

         i=i+strlen(&(mini_display->diff_buf[i]));
      }
   }
}


static int _mini_display_debug_display(struct mini_display_s *mini_display, int buff_id)
{
   char *buf_to_print=NULL;
   char *page_buf=mini_display->pages_buf + (mini_display->display_page*mini_display->c*mini_display->r);

   switch(buff_id)
   {
      case 1:
         buf_to_print=page_buf;
         break;
      case 2:
         buf_to_print=mini_display->display_buf;
         break;
      case 3:
         buf_to_print=mini_display->diff_buf;
         break;
      default:
          return -1;
   }

   int i,j;

   fprintf(stderr,"  ");
   for(i=0;i<mini_display->c;i++)
      fprintf(stderr,"%c",i%10+'0');
   fprintf(stderr,"\n");

   fprintf(stderr," +");
   for(i=0;i<mini_display->c;i++)
      fprintf(stderr,"-");
   fprintf(stderr,"+\n");

   for(j=0;j<mini_display->r;j++)
   {
      fprintf(stderr,"%c|",j%10+'0');
      for(i=0;i<mini_display->c;i++)
      {
         char c=buf_to_print[j*(mini_display->c)+i];
         if((c<' ') || (c>127))
            c='.';
         fprintf(stderr,"%c",c);
      }
      fprintf(stderr,"|\n");
   }

   fprintf(stderr," +");
   for(i=0;i<mini_display->c;i++)
      fprintf(stderr,"-");
   fprintf(stderr,"+\n");
}


static void *_mini_display_thread(void *args)
{
   struct mini_display_s *mini_display=(struct mini_display_s *)args;

   while(1)
   {
      pthread_cleanup_push( (void *)pthread_mutex_unlock, &(mini_display->pages_locks[mini_display->display_page]) );
      pthread_mutex_lock(&(mini_display->pages_locks[mini_display->display_page]));

      if(_mini_display_collect_diff(mini_display)==0)
      {
         _mini_display_commit_diff(mini_display);
         DEBUG_SECTION _mini_display_debug_display(mini_display,2);
      }

      pthread_mutex_unlock(&(mini_display->pages_locks[mini_display->display_page]));
      pthread_cleanup_pop(0);

      _mini_display_send_diff(mini_display);

      usleep(250*1000); // 250 ms
//      usleep(1000*1000); // 250 ms
   }
}


static pthread_t *_mini_display_thread_start(struct mini_display_s *mini_display)
{
   pthread_t *thread=NULL;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
      return NULL;

   if(pthread_create (thread, NULL, (void *)_mini_display_thread, (void *)mini_display))
      return NULL;

   pthread_detach(*thread);

   return thread;
}


int mini_display_clear_screen(struct mini_display_s *mini_display, uint16_t page_num)
{
   int ret=0;
   if(page_num < mini_display->nb_pages)
   {
      char *page_buf=mini_display->pages_buf + (page_num*mini_display->c*mini_display->r);

      pthread_cleanup_push( (void *)pthread_mutex_unlock, &(mini_display->pages_locks[page_num]) );
      pthread_mutex_lock(&(mini_display->pages_locks[page_num]));

      memset(page_buf, (int)' ', mini_display->c*mini_display->r);

      pthread_mutex_unlock(&(mini_display->pages_locks[page_num]));
      pthread_cleanup_pop(0);
   }
   else
      ret=-1;
   return ret;
}


int mini_display_clear_line(struct mini_display_s *mini_display, uint16_t page_num, uint16_t line)
{
   int ret=0;
   if(page_num < mini_display->nb_pages && line < mini_display->r)
   {
      char *page=mini_display->pages_buf + (page_num*mini_display->c*mini_display->r);

      if(line<0 || line>=mini_display->r)
         return -1;

      pthread_cleanup_push( (void *)pthread_mutex_unlock, &(mini_display->pages_locks[page_num]) );
      pthread_mutex_lock(&(mini_display->pages_locks[page_num]));

      memset(page+line*mini_display->c,(int)' ', mini_display->c);
      mini_display->cursors[page_num]=0;

      pthread_mutex_unlock(&(mini_display->pages_locks[page_num]));
      pthread_cleanup_pop(0);
   }
   else
      ret=-1;
   return ret;
}


int mini_display_change_page(struct mini_display_s *mini_display, uint16_t page_num)
{
   int ret=0;

   if(page_num < mini_display->nb_pages)
   {
      pthread_cleanup_push( (void *)pthread_mutex_unlock, &(mini_display->pages_locks[mini_display->display_page]) );
      pthread_mutex_lock(&(mini_display->pages_locks[mini_display->display_page]));

      int current_page=mini_display->display_page;

      mini_display->display_page=page_num;

      pthread_mutex_unlock(&(mini_display->pages_locks[current_page]));
      pthread_cleanup_pop(0);
   }

   return ret;
}


int mini_display_gotoxy(struct mini_display_s *mini_display, uint16_t page_num, uint16_t x, uint16_t y)
{
   int ret=0;
   pthread_cleanup_push( (void *)pthread_mutex_unlock, &(mini_display->pages_locks[page_num]) );
   pthread_mutex_lock(&(mini_display->pages_locks[page_num]));

   int m=(int)(mini_display->c*y+x);

   if(m<(mini_display->c*mini_display->r))
   {
      mini_display->cursors[page_num]=m;
      ret=0;
   }
   else
      ret=-1;

 mini_display_gotoxy_clean_exit:
   pthread_mutex_unlock(&(mini_display->pages_locks[page_num]));
   pthread_cleanup_pop(0);

   return ret;
}


int mini_display_print(struct mini_display_s *mini_display, uint16_t page_num, char *str)
{
   int i;
   if(page_num < mini_display->nb_pages)
   {
      char *page=mini_display->pages_buf + (page_num*mini_display->c*mini_display->r);

      pthread_cleanup_push( (void *)pthread_mutex_unlock, &(mini_display->pages_locks[page_num]) );
      pthread_mutex_lock(&(mini_display->pages_locks[page_num]));

      int max = mini_display->c*mini_display->r;

      for(i=0;str[i] && mini_display->cursors[page_num]<max;i++)
      {
         if(str[i]=='\n')
         {
            mini_display->cursors[page_num]=((mini_display->cursors[page_num]/mini_display->c)+1)*mini_display->c;
         }
         else
            page[(mini_display->cursors[page_num])++]=str[i];
      }

      pthread_mutex_unlock(&(mini_display->pages_locks[page_num]));
      pthread_cleanup_pop(0);
      
      return 0;
   }
   else
      return -1;
}


void mini_display_xy_printf(struct mini_display_s *mini_display, uint16_t page_num, uint16_t x, uint16_t y, int cl_flag, char const* fmt, ...)
{
   va_list args;
   char line[161];

   if(cl_flag==1)
      mini_display_clear_line(mini_display, page_num, y);
   mini_display_gotoxy(mini_display, page_num, x, y);
   va_start(args, fmt);
   vsprintf(line, fmt, args);
   va_end(args);
   mini_display_print(mini_display, page_num, line);
}


int mini_display_set_driver(struct mini_display_s *mini_display, enum driver_type_e type, void *driver)
{
   mini_display->driver.type=0;
   switch(type)
   {
      case LCD:
         mini_display->driver.descriptor.lcd=(struct lcd_s *)driver; 
         break;
      default:
         return -1;
   }
   mini_display->driver.type=type;
   return 0;
}


int mini_display_release(struct mini_display_s *mini_display)
{
   pthread_cancel(*(mini_display->thread));

   if(mini_display->pages_buf!=NULL)
   {
      free(mini_display->pages_buf);
      mini_display->pages_buf=NULL;
   }
   if(mini_display->display_buf!=NULL)
   {
      free(mini_display->display_buf);
      mini_display->display_buf=NULL;
   }
   if(mini_display->diff_buf!=NULL)
   {
      free(mini_display->diff_buf);
      mini_display->diff_buf=NULL;
   }
   if(mini_display->cursors!=NULL)
   {
      free(mini_display->cursors);
      mini_display->cursors=NULL;
   }
}


int mini_display_init(struct mini_display_s *mini_display, int nb_pages, int nb_columns, int nb_rows)
{
   mini_display->c=nb_columns;
   mini_display->r=nb_rows;
   mini_display->nb_pages=nb_pages;
   mini_display->display_page=0;

   mini_display->pages_buf=NULL;
   mini_display->display_buf=NULL;
   mini_display->diff_buf=NULL;
   mini_display->cursors=NULL;

   mini_display->pages_buf=(char *)malloc(nb_columns*nb_rows*nb_pages);
   if(mini_display->pages_buf==NULL)
      goto mini_display_init_clean_exit;

   mini_display->display_buf=(char *)malloc(nb_columns*nb_rows);
   if(mini_display->display_buf==NULL)
      goto mini_display_init_clean_exit;

   mini_display->diff_buf=(char *)malloc(nb_columns*nb_rows);
   if(mini_display->diff_buf==NULL)
      goto mini_display_init_clean_exit;

   mini_display->cursors = (uint16_t *)malloc(sizeof(uint16_t)*nb_pages);
   if(mini_display->cursors==NULL)
      goto mini_display_init_clean_exit;

   mini_display->pages_locks = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*nb_pages);
   if(mini_display->pages_locks==NULL)
      goto mini_display_init_clean_exit;
      
   memset(mini_display->pages_buf,(int)' ',nb_columns*nb_rows*nb_pages);
   memset(mini_display->display_buf,(int)' ',nb_columns*nb_rows);
   memset(mini_display->diff_buf,(int)0,nb_columns*nb_rows);
   memset(mini_display->cursors,(int)0,nb_pages*sizeof(uint16_t));

   int i=0;
   for(;i<nb_pages;i++)
      pthread_mutex_init(&(mini_display->pages_locks[i]), NULL);

   mini_display->thread=_mini_display_thread_start(mini_display);
   if(mini_display->thread==NULL)
      goto mini_display_init_clean_exit;
   else
     return 0;

mini_display_init_clean_exit:
   if(mini_display->pages_buf)
   {
      free(mini_display->pages_buf);
      mini_display->pages_buf=NULL;
   }
   if(mini_display->display_buf)
   {
      free(mini_display->display_buf);
      mini_display->display_buf=NULL;
   }
   if(mini_display->diff_buf)
   {
      free(mini_display->diff_buf);
      mini_display->diff_buf=NULL;
   }
   if(mini_display->cursors)
   {
      free(mini_display->cursors);
      mini_display->cursors=NULL;
   }
   if(mini_display->pages_locks)
   {
      free(mini_display->pages_locks);
      mini_display->pages_locks=NULL;
   }

   mini_display->c=0;
   mini_display->r=0;
   mini_display->nb_pages=0;

   return -1;
}

#ifdef MINIDISPLAY_MODULE_R7
int main(int argc, char *argv[])
{
   struct mini_display_s display;

   mini_display_init(&display, 4, 20, 4);

   sleep(5);
   mini_display_print(&display,0,"TEST\nSAUT");
   sleep(2);
   mini_display_gotoxy(&display,0,4,2);
   sleep(3);
   mini_display_clear(&display,0);
   mini_display_print(&display,0,"MILIEU 3 EME");
   sleep(4);

   int i;
   char str[161];

   for(i=0;i<10;i++)
   {
      mini_display_gotoxy(&display,0,0,2);
      sprintf(str,"%d",i); 
      mini_display_print(&display,0,str);
      sleep(1);
   }
   
   mini_display_release(&display);

  sleep(5);
}
#endif
