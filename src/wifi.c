#include <stdio.h>
#include <string.h>

#include "wifi.h"

#include "mea_utils.h"
#include "mea_string_utils.h"
#include "mea_verbose.h"

//
// Récupération des "access points" (ap)
//
struct iwscan_state_s
{
  int ap_num; // Access Point number 1->N
};


static int _count_essid(unsigned char *buffer, int l_buffer, struct iw_range *range)
 /**
  * \brief     compte le nombre d' "essid" contenu dans un buffer d'événements iw en provenance du noyau
  * \param     buffer    données brutes en provenance du noyau
  * \param     l_buffer  taille de la zone de données (en octet)
  * \param     range     
  * \return    nombre d'essid
  */
{
   int cntr = 0;
   struct iw_event iwe;
   struct stream_descr stream;
   int ret;

   iw_init_event_stream(&stream, (char *)buffer, l_buffer);
   do
   {
      ret = iw_extract_event_stream(&stream, &iwe, range->we_version_compiled);
      if(ret > 0)
      {
         if(iwe.cmd == SIOCGIWAP)
            cntr++;
      }
   }
   while(ret > 0);
   
   return cntr;
}


static int _ap_data_compare(void const *a, void const *b)
 /**
  * \brief     compare 2 ap_data (pour qsort)
  * \details   la comparaison se fait sur une combinaison quality/level avec en priorité la quality (1er critère).
  * \param     a,b  les deux ap_data à comparer
  * \return    <0 (a>b), >0 (a<b), 0 (égal).
  */
{
   struct ap_data_s *pa,*pb;

   pa=(struct ap_data_s *)a;
   pb=(struct ap_data_s *)b;

   return (int)( ((pb->quality * 1000.0) + pb->lvl) - ((pa->quality * 1000.0) + pa->lvl) );
}


static int _iw_calc_signal_info(const iwqual *qual,
                                const iwrange *range,
                                float *signal_quality,
                                float *signal_level)
{
   int len;

   if(qual->updated & IW_QUAL_QUAL_INVALID)
   {
      *signal_quality=-1.0;
      *signal_level=0.0;
      return -1;
   }

   *signal_quality=(float)qual->qual / (float)range->max_qual.qual;
   *signal_quality=(*signal_quality < 1.0) ? *signal_quality : 1.0;

   if((qual->level != 0) || (qual->updated & (IW_QUAL_DBM | IW_QUAL_RCPI)))
   {
      // Deal with quality : always a relative value
      // Check if the statistics are in RCPI (IEEE 802.11k)
      if(qual->updated & IW_QUAL_RCPI)
      {
         // Deal with signal level in RCPI
         // RCPI = int{(Power in dBm +110)*2} for 0dbm > Power > -110dBm
         double rcpilevel = (qual->level / 2.0) - 110.0;
         *signal_level = (float)rcpilevel;
      }
      else
      {
          // Check if the statistics are in dBm
         if((qual->updated & IW_QUAL_DBM) || (qual->level > range->max_qual.level))
         {
            // Deal with signal level in dBm  (absolute power measurement)
            int dblevel = qual->level;

            // Implement a range for dBm [-192; 63]
            if(qual->level >= 64)
               dblevel -= 0x100;
            *signal_level=(float)dblevel;
         }
         else
         {
            // Deal with signal level as relative value (0 -> max)
            *signal_level=(float)qual->level;
         }
      }
      return 0;
   }
   else
   {
      *signal_quality=(float)qual->qual;
      *signal_level=(float)qual->level;
      return 1;
   }
}


static int _ap_data_update(struct stream_descr *stream, // Stream of events
                           struct iw_event *event, // Extracted token
                           struct iwscan_state_s *state,
                           struct iw_range *iw_range, // Range info
                           struct ap_data_s *ap_data,
                           int    ap_data_l)
{
   char buffer[128];

   // Now, let's decode the event
   switch(event->cmd)
   {
      case SIOCGIWNWID:
      case SIOCGIWFREQ:
      case SIOCGIWMODE:
      case SIOCGIWNAME:
      case SIOCGIWENCODE:
      case SIOCGIWRATE:
      case SIOCGIWMODUL:
      case IWEVGENIE:
      case IWEVCUSTOM:
         return -1;
         break;

      case SIOCGIWAP:
         state->ap_num++;
         ap_data[state->ap_num-1].index=state->ap_num-1;
         return 1;
         break;

      case SIOCGIWESSID:
      {
         char essid[IW_ESSID_MAX_SIZE+1];
 
         memset(essid, '\0', sizeof(essid));

         if((event->u.essid.pointer) && (event->u.essid.length))
            memcpy(essid, event->u.essid.pointer, event->u.essid.length);
         if(event->u.essid.flags)
            strncpy(ap_data[state->ap_num-1].essid, essid, sizeof(ap_data->essid)-1);
         else
         {
         }
         return 2;
      }
      break;

      case IWEVQUAL:
      {
         float quality,lvl;
         _iw_calc_signal_info(&event->u.qual, iw_range, &quality, &lvl);
         ap_data[state->ap_num-1].quality=quality;
         ap_data[state->ap_num-1].lvl=lvl;

         return 3;
      }
      break;

      default:
         return -1;
         break;
   }
}


struct ap_data_s *ap_data_get_alloc(int skfd, char *ifname, char *searched_essid, int last)
{
   struct iwreq       wrq;
   struct iw_scan_req scanopt; // Options for 'set'
   int                scanflags = 0; // Flags for scan
   unsigned char     *buffer = NULL; // Results 
   int                buflen = IW_SCAN_MAX_DATA; // Min for compat WE<17
   struct iw_range    range;
   int                has_range;
   struct timeval     tv; // Select timeout
   int                timeout = 15000000; // 15s
   struct ap_data_s  *ap_data = NULL;


   // recupération des "ranges"
   has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);

   // Check if the interface could support scanning.
   if((!has_range) || (range.we_version_compiled < 14))
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : %-8.16s interface doesn't support scanning.\n", DEBUG_STR, __func__, ifname);
      return NULL;
   }

   /* Init timeout value -> 250ms between set and first get */
   tv.tv_sec = 0;
   tv.tv_usec = 250000;

   /* Clean up set args */
   memset(&scanopt, 0, sizeof(scanopt));

   if(searched_essid)
   {
      // Store the ESSID in the scan options
      scanopt.essid_len = strlen(searched_essid);
      memcpy(scanopt.essid, searched_essid, scanopt.essid_len);
      
      // Initialise BSSID as needed
      if(scanopt.bssid.sa_family == 0)
      {
         scanopt.bssid.sa_family = ARPHRD_ETHER;
         memset(scanopt.bssid.sa_data, 0xff, ETH_ALEN);
      }
      // Scan only this ESSID
      scanflags |= IW_SCAN_THIS_ESSID;
   }

   if(last)
      scanflags |= IW_SCAN_HACK;

   // Check if we have scan options
   if(scanflags)
   {
      wrq.u.data.pointer = (caddr_t) &scanopt;
      wrq.u.data.length = sizeof(scanopt);
      wrq.u.data.flags = scanflags;
   }
   else
   {
      wrq.u.data.pointer = NULL;
      wrq.u.data.flags = 0;
      wrq.u.data.length = 0;
   }

   if(scanflags == IW_SCAN_HACK)
   {
      /* Skip waiting */
      tv.tv_usec = 0;
   }
   else
   {
      // Initiate Scanning
      if(iw_set_ext(skfd, ifname, SIOCSIWSCAN, &wrq) < 0)
      {
         if((errno != EPERM) || (scanflags != 0))
         {
            DEBUG_SECTION mea_log_printf("%s (%s) : %-8.16s interface doesn't support scanning : %s\n", DEBUG_STR, __func__, ifname, strerror(errno));
            return NULL;
         }
         tv.tv_usec = 0;
      }
   }
   timeout -= tv.tv_usec;

   while(1)
   {
      fd_set rfds; // File descriptors for select
      int last_fd; // Last fd
      int ret;

      FD_ZERO(&rfds);
      last_fd = -1;

      // Wait until something happens
      ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);

      // Check if there was an error
      if(ret < 0)
      {
         if(errno == EAGAIN || errno == EINTR)
	         continue;
         DEBUG_SECTION mea_log_printf("%s (%s) : unhandled signal - exiting...\n", DEBUG_STR, __func__);
         return NULL;
      }

      // Check if there was a timeout
      if(ret == 0)
      {
         unsigned char *newbuf;

realloc:
	// (Re)allocate the buffer - realloc(NULL, len) == malloc(len)
	newbuf = realloc(buffer, buflen);
	if(newbuf == NULL)
	{
           DEBUG_SECTION mea_log_printf("%s (%s) : allocation failed\n", DEBUG_STR, __func__);
	   if(buffer)
              free(buffer);
	   return NULL;
	}
	buffer = newbuf;

	 // Try to read the results
   wrq.u.data.pointer = buffer;
   wrq.u.data.flags = 0;
   wrq.u.data.length = buflen;
   if(iw_get_ext(skfd, ifname, SIOCGIWSCAN, &wrq) < 0)
   {
      // Check if buffer was too small (WE-17 only)
      if((errno == E2BIG) && (range.we_version_compiled > 16))
      {
         /*
          * Some driver may return very large scan results, either
          * because there are many cells, or because they have many
          * large elements in cells (like IWEVCUSTOM). Most will
          * only need the regular sized buffer. We now use a dynamic
          * allocation of the buffer to satisfy everybody. Of course,
          * as we don't know in advance the size of the array, we try
          * various increasing sizes. Jean II
          */

         // Check if the driver gave us any hints.
         if(wrq.u.data.length > buflen)
            buflen = wrq.u.data.length;
         else
            buflen *= 2;

            goto realloc;
         }

         // Check if results not available yet
         if(errno == EAGAIN)
         {
            // Restart timer for only 100ms
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            timeout -= tv.tv_usec;
            if(timeout > 0)
               continue;
         }
         DEBUG_SECTION mea_log_printf("%s (%s) : %-8.16s failed to read scan data : %s\n", DEBUG_STR, __func__, ifname, strerror(errno));

         free(buffer);
	      return NULL;
      }
      else
	   // We have the results, go to process them
	      break;
      }
   }

   if(wrq.u.data.length)
   {
      int nb_essid=0;
      struct iw_event iwe;
      struct stream_descr stream;
//      struct iwscan_state_s state = { .ap_num = 0, .val_index = 0 };
      struct iwscan_state_s state = { .ap_num = 0 };
      int ret;

      // combien d'essid dans la liste des événements ?
      nb_essid=_count_essid((char *)buffer, wrq.u.data.length, &range);

      // allocation du bon nombre d'essid et initialisation
      ap_data = (struct ap_data_s *)malloc((nb_essid+1) * sizeof(struct ap_data_s));
      if(!ap_data)
      {
         free(buffer);
         buffer=NULL;
         return NULL;
      }
      int i=0;
      for(;i<nb_essid+1;i++)
      {
         ap_data[i].index=-1;
         ap_data[i].essid[0]=0;
         ap_data[i].quality=-1000;
         ap_data[i].lvl=-1000;
      }

      iw_init_event_stream(&stream, (char *) buffer, wrq.u.data.length);
      do
      {
         // Extract an event and print it
         ret = iw_extract_event_stream(&stream, &iwe, range.we_version_compiled);
         if(ret > 0)
            _ap_data_update(&stream, &iwe, &state, &range, ap_data, nb_essid);
      }
      while(ret > 0);
      
      if(ap_data)
         qsort(ap_data, nb_essid, sizeof(struct ap_data_s), _ap_data_compare);
   }
   else
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : %-8.16s no scan results\n", DEBUG_STR, __func__, ifname);
   }
   free(buffer);

   return ap_data;
}


int wifi_ap_find(char *iface, char *essid_searched, float *qmax, float *lmax, float *qmin, float *lmin)
{
   int skfd; // generic raw socket desc
   char *dev; // device name
   char *cmd; // command

   // Create a channel to the NET kernel
   if((skfd = iw_sockets_open()) < 0)
   {
      perror("socket");
      return -1;
   }

   int nb_essid_found=0;
   float _qmax,_lmax,_qmin,_lmin;

   struct ap_data_s *ap_data=ap_data_get_alloc(skfd, iface, essid_searched, 0);
   if(ap_data)
   {
      int i=0;
      // max
      for(;ap_data[i].index!=-1 && nb_essid_found==0;i++)
      {
         if(essid_searched==NULL || strcmp(essid_searched, ap_data[i].essid)==0)
         {
            nb_essid_found++;
            _qmax=ap_data[i].quality;
            _lmax=ap_data[i].lvl;
            _qmin=ap_data[i].quality;
            _lmin=ap_data[i].lvl;
            DEBUG_SECTION mea_log_printf("%s (%s) : %03d ESSID=%s (%.2f%%,%.2fdb)\n", INFO_STR, __func__, ap_data[i].index, ap_data[i].essid, ap_data[i].quality*100.0, ap_data[i].lvl);
         }
      }
      // min
      for(;ap_data[i].index!=-1;i++)
      {
         if(essid_searched==NULL || essid_searched[0]==0 || strcmp(essid_searched, ap_data[i].essid)==0)
         {
            nb_essid_found++;
            _qmin=ap_data[i].quality;
            _lmin=ap_data[i].lvl;
            DEBUG_SECTION mea_log_printf("%s (%s) : %03d ESSID=%s (%.2f%%,%.2fdb)\n", INFO_STR, __func__, ap_data[i].index, ap_data[i].essid, ap_data[i].quality*100.0, ap_data[i].lvl);
         }
      }
      free(ap_data);
   }
   else
   {
      iw_sockets_close(skfd);
      return -1;
   }

  // Close the socket
   iw_sockets_close(skfd);

   if(qmax)
      *qmax=_qmax;
   if(lmax)
      *lmax=_lmax;
   if(qmax)
      *qmin=_qmin;
   if(lmin)
      *lmin=_lmin;

   return nb_essid_found;
}


int wifi_connection_status(char *iface, float *q, float *l)
{
/*
 # format du pseudo fichier /proc/net/wireless
 #Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
 # face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
 # wlan0: 0000    0     0     0        0      0      0      0      0        0
 # wlan1: 0000   44.  -75.  -256.       0      0      0      0      0        0
 */
/*
 * voir aussi /sys/class/net/wlanX pour avoir des info sur la connexion
 */
   FILE *fd;
   char line[256];
   char _iface[20];
   char _status[5];
   float _q,_l;
   int ret=-1;

   fd=fopen("/proc/net/wireless","r");
   if(fd==NULL)
      return -1;

   while(!feof(fd))
   {
      if(fgets(line, sizeof(line)-1, fd))
      {
         sscanf(line,"%20[^:]: %s %f %f",_iface,_status,&_q,&_l);
         if(strcmp(iface, mea_strtrim(_iface))==0)
         {
//            DEBUG_SECTION fprintf(stderr,"%s %s %f %f\n",mea_strtrim(_iface),_status,_q,_l);
            *q=_q;
            *l=_l;
            ret=0;
            break;
         }
      }
   }

   fclose(fd);

   return ret;
}


int waitessid(char *iface, char *essid, float min_quality, int timeout)
{
   float qmax, qmin, lmax, lmin;

   for(;;)
   {
      int ret=wifi_ap_find(iface, essid, &qmax, &lmax, &qmin, &lmin);
      if(ret<0)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : scanning error !!!\n", DEBUG_STR, __func__);
         return -1;
      }
      if(ret>0)
      {
         if(min_quality>0)
         {
            if(qmax<min_quality)
               continue;
         }
         DEBUG_SECTION mea_log_printf("%s (%s) : %d x %s found - max(%2.0f%% / %2.0fdb), min(%2.0f%% / %2.0fdb)\n",DEBUG_STR,__func__, ret, essid, qmax*100.0, lmax, qmin*100.0, lmin);
         return ret;
      }
   }
   sleep(1);
}
