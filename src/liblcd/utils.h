#ifndef __utils_h
#define __utils_h

#include <inttypes.h>

/**
  * \brief     copie la valeur d'un bit d'une variable vers un autre bit d'une autre variable.
  * \param     variable (ou valeur) source
  * \param     numéro du bit à copier
  * \param     variable destination
  * \param     numéro du bit à destination
  */
#define BITCPY(S, s, D, d) D = (S) & (1 << (s)) ? D | (1 << (d)) : D & ~(1 << (d))


 /**
  * \brief     position un bit à 1 dans une variable
  * \param     variable à modifier
  * \param     numéro du bit à positionner à 1
  */
#define BITSET(b, n) b |= (1 << (n))


 /**
  * \brief     position un bit à 0 dans une variable
  * \param     variable à modifier
  * \param     numéro du bit à positionner à 0
  */
#define BITCLEAR(b, n) b &= ~(1 << (n))


void mynanosleep(uint32_t ns);
void mymicrosleep(uint32_t us);

int16_t strcmplower(char *str1, char *str2);
int16_t strsplit(char str[], char separator, char *tokens[], char l_tokens);
char *strltrim(char *s);
char *strrtrim(char *s);
char *strtrim(char *s);
void strtolower(char *str);
 
#endif


