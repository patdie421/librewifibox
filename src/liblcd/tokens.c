#include <stdio.h>

#include "const.h"
#include "utils.h"
#include "tokens.h"
#include "hd44780.h"


int getTokenID(struct token_s tokens_list[], char *str)
{
   if(!str)
      return -1;
 
   uint16_t i;
   for(i=0;tokens_list[i].str;i++)
   {
      if(strcmplower(tokens_list[i].str, str) == 0)
         return tokens_list[i].id;
   }
   return -1;
}
