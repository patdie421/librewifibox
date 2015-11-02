#ifndef __mea_verbose_h
#define __mea_verbose_h

#include <pthread.h>
#include <stdarg.h>

#define ERROR_STR        _error_str
#define INFO_STR         _info_str
#define DEBUG_STR        _debug_str
#define FATAL_ERROR_STR  _fatal_error_str
#define WARNING_STR      _warning_str
#define MALLOC_ERROR_STR _malloc_error_str

#define DEBUG_SECTION if(debug_msg)
#define VERBOSE(v) if(v <= verbose_level)
#define MEA_STDERR stderr

extern int verbose_level;
extern debug_msg;

extern const char *_error_str;
extern const char *_info_str;
extern const char *_debug_str;
extern const char *_fatal_error_str;
extern const char *_warning_str;
extern const char *_malloc_error_str;

#define PRINT_MALLOC_ERROR { mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR); perror(""); }

void mea_log_printf(char const* fmt, ...);
pthread_rwlock_t *mea_log_get_rwlock();

#endif
