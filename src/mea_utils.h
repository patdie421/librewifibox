#ifndef __mea_utils_h
#define __mea_utils_h

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdarg.h>


typedef struct mea_timer_s
{
   uint16_t stat; // (0 = inactive, 1=active)
   time_t start_time;
   uint32_t delay; // timer delay
   uint16_t autorestart; // (0 = no, 1=yes)
} mea_timer_t;


uint16_t mea_init_timer(mea_timer_t *aTimer, uint32_t aDelay, uint16_t restartStatus);
void mea_start_timer(mea_timer_t *aTimer);
void mea_stop_timer(mea_timer_t *aTimer);
int16_t mea_test_timer(mea_timer_t *aTimer);
void mea_log_printf(char const* fmt, ...);

#ifdef MEA_STRING
char *mea_strltrim(char *s);
char *mea_strrtrim(char *s);
char *mea_strtrim(char *s);
int mea_strncat(char *dest, int max_test, char *source);
void mea_strremovespaces(char *str);
#endif

double mea_now();

#endif
