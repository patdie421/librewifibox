#ifndef __tokens_h
#define __tokens_h

struct token_s
{
   char *str;
   int id;
};

int getTokenID(struct token_s tokens_list[], char *str);

#endif
