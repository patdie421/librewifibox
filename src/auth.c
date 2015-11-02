#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>

#include "curl_adds.h"

#include "mea_verbose.h"


int freewifi_auth_validation(char *url)
{
   CURL *curl;

   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_NOBODY ,1L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_HEADERDATA, &cr);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      DEBUG_SECTION mea_log_printf("%s (%s) : auth validation with %s ...\n", DEBUG_STR, __func__, url);
 
      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed: %s\n", curl_easy_strerror(res), DEBUG_STR, __func__);
         ret=-1;
      }
      else
      {
//         DEBUG_SECTION fprintf(stderr, "%s\n", cr.p);
         if(strstr(cr.p, "Location: https://wifi.free.fr/"))
         {
            DEBUG_SECTION mea_log_printf("%s (%s) : redirect to https://wifi.free.fr => probably not logged in\n", DEBUG_STR, __func__);
            ret=1;
         }
         else
         {
            DEBUG_SECTION mea_log_printf("%s (%s) : authentication ok\n", DEBUG_STR, __func__);
            ret=0;
         }
      }

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);
   return ret;
}


int freewifi_auth(char *login, char *password)
{
   CURL *curl;

   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      char post_data[255];

      sprintf(post_data, "login=%s&password=%s&submit=Valider", login, password);
      curl_easy_setopt(curl, CURLOPT_URL, "https://wifi.free.fr/Auth");

//      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      DEBUG_SECTION mea_log_printf("%s (%s) : try authentification ... (%d)\n", DEBUG_STR, __func__,time(NULL));

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         ret=-1;
      }
      else
      {
         if(strstr(cr.p,"CONNEXION AU SERVICE REUSSIE") || strstr(cr.p,"INTERFACE DE GESTION"))
         {
            DEBUG_SECTION mea_log_printf("%s (%s) : authentication done (%d)\n", DEBUG_STR, __func__, time(NULL));
            ret=0;
         }
         else
         {
            DEBUG_SECTION mea_log_printf("%s (%s) : authentication failed (%d)\n", DEBUG_STR, __func__, time(NULL));
            ret=1;
         }
      }

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);
   return ret;
}

