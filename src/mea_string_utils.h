//
//  string_utils.h
//
//  Created by Patrice DIETSCH on 08/07/2013.
//
//

#ifndef __mea_string_utils_h
#define __mea_string_utils_h

#include <inttypes.h>

void    mea_strtoupper(char *str);
void    mea_strtolower(char *str);

int16_t mea_strcmplower(char *str1, char *str2);
int16_t mea_strncmplower(char *str1, char *str2, int n);

char   *mea_strltrim(char *s);
char   *mea_strltrim2(char *s);
char   *mea_strrtrim(char *s);
char   *mea_strtrim(char *s);
char   *mea_strltrim_alloc(char *s);
char   *mea_strrtrim_alloc(char *s);
char   *mea_strtrim_alloc(char *s);

int16_t mea_strsplit(char str[], char separator, char *tokens[], uint16_t l_tokens);
size_t  mea_snprintfcat(char* buf, size_t bufSize, char const* fmt, ...);
int     mea_strncat(char *dest, int max_test, char *source);
void    mea_strremovespaces(char* str);

char   *mea_string_free_alloc_and_copy(char **org_str, char *str);
char   *mea_string_alloc_and_copy(char *str);

int     mea_strisnumeric(char *str);

#endif
