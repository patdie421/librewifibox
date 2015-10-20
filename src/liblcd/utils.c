#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "const.h"
#include "utils.h"

void mynanosleep(uint32_t ns)
{
   struct timespec req,res;

   req.tv_sec=0;
   req.tv_nsec=ns;

   while ( nanosleep(&req,&res) == -1 )
   {
      req.tv_sec  = res.tv_sec;
      req.tv_nsec = res.tv_nsec;
   }
}


void mymicrosleep(uint32_t usecs)
{
   struct timespec delay_time,remaining;

   delay_time.tv_sec = 0;
   delay_time.tv_nsec = usecs * 1000;
   while ( nanosleep(&delay_time,&remaining) == -1 )
   {
      delay_time.tv_sec  = remaining.tv_sec;
      delay_time.tv_nsec = remaining.tv_nsec;
   }
}


int16_t strcmplower(char *str1, char *str2)
{
   int i;

   if(!str1 || !str2)
      return 0;

   for(i=0;str1[i];i++) {
      if(tolower(str1[i])!=tolower(str2[i]))
         return 1;
   }
   if(str1[i]!=str2[i])
      return 1;
   return 0;
}


int16_t strsplit(char str[], char separator, char *tokens[], char l_tokens)
{
   int i=0,j=0;
   
   tokens[j]=str;
   for(i=0;str[i];i++)
   {
      if(str[i]==separator)
      {
         str[i]=0;
         j++;
         if(j<l_tokens)
            tokens[j]=&(str[i+1]);
         else
            return -1;
      }
   }
   return j+1;
}


char *strltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}


char *strrtrim(char *s)
{
    char *back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}


char *strtrim(char *s)
{
    return strrtrim(strltrim(s));
}


void strtolower(char *str)
{
   uint16_t i;
   for(i=0;i<strlen(str);i++)
      str[i]=tolower(str[i]);
}
