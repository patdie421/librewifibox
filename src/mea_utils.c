#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>

#include "mea_utils.h"


double mea_now()
{
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    double milliseconds = (double)te.tv_sec*1000.0 + (double)te.tv_usec/1000.0; // caculate milliseconds

    return milliseconds;
}


uint16_t mea_init_timer(mea_timer_t *aTimer, uint32_t aDelay, uint16_t restartStatus)
/**
 * \brief     création (initialisation) d'un timer
 * \details   initialise une structure "mea_timer_t". Le timer est "arrêté" au démarrage (pas de prise de chrono)
 * \param     aTimer   le timer.
 * \param     aDelay   délai avant expiration du timer.
 * \param     restartStatus   type de rearmement du timer.
 * \return    0 initialisation réalisée, -1 sinon
 */
{
   if(aTimer)
   {
     aTimer->stat=0;
     aTimer->start_time=0;
     aTimer->delay=aDelay;
     aTimer->autorestart=restartStatus;
 
     return 0;
   }
   else
     return -1;
}


void mea_start_timer(mea_timer_t *aTimer)
/**
 * \brief     lancement d'un timer
 * \details   prise de chrono.
 * \param     aTimer   le timer.
 */
{
   time(&(aTimer->start_time));
   aTimer->stat=1;
}


void mea_stop_timer(mea_timer_t *aTimer)
/**
 * \brief     arrêt du timer
 * \details   Le chrono est remis à 0
 * \param     aTimer   le timer.
 */
{
   aTimer->start_time=0;
   aTimer->stat=0;
}


int16_t mea_test_timer(mea_timer_t *aTimer)
/**
 * \brief     test le timer
 * \details   teste si le delai est dépassé.
 * \param     aTimer   le timer.
 * \return    0 le temps est dépassé (le temps est passé), -1 sinon
 */
{
   if(aTimer->stat==1)
   {
      time_t now;
      double diff_time;
	  
      time(&now);
	    
      diff_time=difftime(now,aTimer->start_time);
      if(diff_time > (double)(aTimer->delay))
      {
         if(aTimer->autorestart==1)
         {
            time(&(aTimer->start_time));
         }
         else
         {
            aTimer->stat=0;
            aTimer->start_time=0;
         }
         return 0;
      }
      else
         return -1;
   }
   else
      return -1;
}	  

#ifdef MEA_STRING
char *mea_strltrim(char *s)
/**
 * \brief     trim (suppression de blanc) à gauche d'une chaine.
 * \details   la chaine n'est pas modifiée. un pointeur est retourné sur le premier caractère "non blanc"
 *            de la chaine. Attention à ne pas perdre le pointeur d'origine pour les chaines allouées.
 *            Eviter 'absolument s=mea_strltrim(s)', préférez 's_trimed=strltrim(s)'.
 * \param     s  chaine à trimer
 * \return    pointeur sur le premier caractère "non blanc" de la chaine
 */
{
   if(!s)
      return NULL;

   while(*s && isspace(*s)) s++;
   return s;
}


char *mea_strrtrim(char *s)
/**
 * \brief     trim (suppression de blanc) à droite d'une chaine.
 * \details   la chaine est modifiée. un '\0' est positionné à la place du premier des "blancs" en fin de chaine.
 * \param     s  chaine à trimer
 * \return    pointeur sur le début de la chaine ou NULL (si s=NULL)
 */
{
   if(s!=NULL && s[0]!=0) // chaine vide
   {
      char *back = s + strlen(s);
      while(isspace(*--back));
      *(back+1) = '\0';
   }
   return s;
}


char *mea_strtrim(char *s)
/**
 * \brief     trim (suppression de blanc) à gauche et à droite d'une chaine.
 * \details   la chaine est modifiée, un '\0' un '\0' est positionné à la place du premier "blanc"
 *            des blancs en fin de chaine (voir mea_strrtrim()) et un pointeur est retourné sur le
 *            premier caractère "non blanc" de la chaine (voir mea_strltrim())
 * \param     s  chaine à trimer
 * \return    pointeur sur le début de la chaine.
 */
{
    return mea_strrtrim(mea_strltrim(s));
}


int mea_strncat(char *dest, int max_test, char *source)
{
/**
 * \brief     fait un "strcat" avec limitation de la taille de la chaine rÃ©sultante
 * \param     dest       pointeur sur la chaine Ã  complÃ©ter.
 * \param     bufSize    taille max que peut contenir la chaine Ã  complÃ©tÃ©
 * \param     source     pointeur sur la chaine Ã  rajouter Ã  dest
 * \return    -1 si la taille de dest ne peut pas contenir source (dest n'est pas modifiÃ©), 0 sinon
 */
   int l_dest = strlen(dest);
   int l_source = strlen(source);
   if((l_dest+l_source)>max_test)
      return -1;
   else
   {
      strcat(dest, source);
   }
   return 0;
}


/**
 * \brief     supprime tous les espaces d'une chaine
 * \param     str        pointeur sur la chaine Ã  traiter.
 */
void mea_strremovespaces(char *str)
{
   char c, *p;

   p = str;
   do
      while ((c = *p++) == ' ');
   while (*str++ = c);
   return;
}
#endif
