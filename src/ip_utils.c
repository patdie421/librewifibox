#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <poll.h>

#include "mea_verbose.h"

#include "ip_utils.h"


#define PACKETSIZE  64

struct packet
{
    struct icmphdr hdr;
    char msg[PACKETSIZE-sizeof(struct icmphdr)];
};


int getifaceaddr(char *iface, char *addr)
{
   struct ifaddrs *ifaddr, *ifa;
   int family, s;
   char host[NI_MAXHOST];
   int ret=-1;

   if (getifaddrs(&ifaddr) == -1) 
   {
      perror("getifaddrs");
      return -1;
   }

   for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
   {
      if (ifa->ifa_addr == NULL)
         continue;  

      if (ifa->ifa_addr->sa_family!=AF_INET)
         continue;

      s=getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (s != 0)
         continue;

      if(strcmp(ifa->ifa_name, iface)==0)
      {
/*
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : interface : <%s>\n", DEBUG_STR, __func__, ifa->ifa_name);
            mea_log_printf("%s (%s) : address   : <%s>\n", DEBUG_STR, __func__, host);
         }
*/
         if(addr)
            strcpy(addr, host);
         ret=0;
         goto ip_clean_exit;
      }
   }

ip_clean_exit:
   if(ifaddr)
   {
      freeifaddrs(ifaddr);
      ifaddr=NULL;
   }
   return ret;
}


static unsigned short _ping_checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

    for(sum=0;len>1;len-=2)
       sum+=*buf++;
    if(len==1)
       sum+=*(unsigned char*)buf;
    sum=(sum >> 16) + (sum & 0xFFFF);
    sum+=(sum >> 16);
    result=~sum;

    return result;
}


int ping(char *address)
{
   const int val=255;
   int i, sd;
   struct packet pckt;
   struct sockaddr_in r_addr;
   struct hostent *hname;
   struct protoent *proto=NULL;
   struct sockaddr_in addr_ping,*addr;

   int loop1, loop2;

   int pid = getpid();

   proto = getprotobyname("ICMP");
   if(proto==NULL)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : can't get proto - ", INFO_STR, __func__);
         perror("");
      }
      return -2;
   }

   hname = gethostbyname(address);
   if(hname==NULL)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : can't get host address - ", INFO_STR, __func__);
         perror("");
      }
      return -3;
   }

   bzero(&addr_ping, sizeof(addr_ping));
   addr_ping.sin_family = hname->h_addrtype;
   addr_ping.sin_port = 0;
   addr_ping.sin_addr.s_addr = *(long*)hname->h_addr;

   addr = &addr_ping;

   sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
   if(sd<0)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : socket - ", INFO_STR, __func__);
         perror("");
      }
      return -4;
   }

   if (setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : set TTL option - ", INFO_STR, __func__);
         perror("");
      }
      close(sd);
      return -5;
   }

   if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : request nonblocking I/O - ", INFO_STR, __func__);
         perror("");
      }
      close(sd);
      return -6;
   }

   for(loop1=0;loop1 < 5;loop1++)
   {
      bzero(&pckt, sizeof(pckt));
      pckt.hdr.type = ICMP_ECHO;
      pckt.hdr.type = ICMP_ECHO;
      pckt.hdr.un.echo.id = pid;
      for (i=0; i < sizeof(pckt.msg)-1;i++)
         pckt.msg[i] = i+'0';
      pckt.msg[i] = 0;
      pckt.hdr.un.echo.sequence = loop1;
      pckt.hdr.checksum = _ping_checksum(&pckt, sizeof(pckt));

      if(sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0)
      {
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : sendto - ", INFO_STR, __func__);
            perror("");
            close(sd);
            return -1;
         }
      }

      loop2=0;
      for(loop2=0;loop2<10;loop2++)
      {
         int len=sizeof(r_addr);
         if(recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) > 0)
         {
            close(sd);
            return 0;
         }
         usleep(50000);
      }
    }

    close(sd);
    return -1;
}


int waitipaddr(char *iface, int timeout, char *addr)
{
   int i=0;
   for(;i<timeout;i++)
   {
      if(getifaceaddr(iface, addr)==0)
      {
         return 0;
      }
      sleep(1);
   }
   return -1;
}
