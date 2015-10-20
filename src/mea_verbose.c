#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "mea_verbose.h"

int verbose_level=10;
int debug_msg=1;

const char *_error_str = "ERROR";
const char *_info_str  = "INFO ";
const char *_debug_str = "DEBUG";
const char *_fatal_error_str="FATAL ERROR";
const char *_warning_str="WARNING";
const char *_malloc_error_str="malloc error";

void mea_log_printf(char const* fmt, ...)
/**
 * \brief     imprime un message de type log
 * \details   un "timestamp" est afficher avant le message. Pour le reste s'utilise comme printf.
 * \param     _last_time   valeur retournée par start_chrono
 * \return    différence entre "maintenant" et _last_chrono en milliseconde
 */
{
   va_list args;
   static char *date_format="[%Y-%m-%d %H:%M:%S]";

   char date_str[40];
   time_t t;
   struct tm t_tm;

   t=time(NULL);

   if (localtime_r(&t, &t_tm) == NULL)
   {
//      perror("");
      return;
   }

   if (strftime(date_str, sizeof(date_str), date_format, &t_tm) == 0)
   {
//      fprintf(MEA_STDERR, "strftime returned 0");
      return;
   }

   fprintf(MEA_STDERR, "%s ", date_str);
   
   va_start(args, fmt);
   vfprintf(MEA_STDERR, fmt, args);
   va_end(args);
}


