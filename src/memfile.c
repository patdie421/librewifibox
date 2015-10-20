#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "memfile.h"


struct memfile_s *memfile_alloc()
{
   struct memfile_s *mf;

   mf=(struct memfile_s *)calloc(1, sizeof(struct memfile_s));

   return mf;
}


struct memfile_s *memfile_init(struct memfile_s *mf, enum mf_type_e type, uint32_t bs, uint32_t nb)
{
   if(mf==NULL)
      return NULL;

   mf->block_size=bs;
   mf->block_num=nb;
   mf->type=type;
   mf->p=0;

   mf->data=(char *)calloc(nb,bs);

   if(mf->data==NULL)
      return NULL;

   return mf;
}


int memfile_release(struct memfile_s *mf)
{
   mf->block_size=0;
   mf->block_num=0;
   mf->type=0;
   mf->p=0;
 
   if(mf->data)
   {
      free(mf->data);
      mf->data=NULL;
   }
}


int memfile_seek(struct memfile_s *mf, uint32_t p)
{
   if(p < (mf->block_size*mf->block_num))
   {
      mf->p = p;
      return 0;
   }
   else
   {
      return -1;
   }
}


int memfile_extend(struct memfile_s *mf, uint32_t nb)
{
   if(mf->type != AUTOEXTEND)
      return -1;
   else
   {
      char *tmp=mf->data;

      mf->data=(char *)realloc(mf->data, (mf->block_num+nb)*mf->block_size);
      if(mf->data==NULL)
      {
         mf->data=tmp;
         return -1;
      }
      else
      {
         memset(&(mf->data[mf->block_num*mf->block_size]),0,mf->block_size*nb);
         mf->block_num= mf->block_num+nb;

         return 0;
      }
   }
   return -1;
}


int memfile_include(struct memfile_s *mf, char *file, int zero_ended)
{
   FILE *fd;

   fd=fopen(file, "r");
   if(!fd)
   {
      perror("");
      return -1;
   }

   char buf[512];

   while(!feof(fd))
   {
      size_t nb=fread(buf, 1, sizeof(buf), fd);
      int i=0;
      for(;i<nb;i++)
      {
         if(mf->p >= mf->block_size*mf->block_num)
         {
            if(memfile_extend(mf, 5)<0)
            {
               fclose(fd);
               return 1;
            }
         }
         mf->data[(mf->p)++]=buf[i];
      }

      if(zero_ended)
      {
         if(mf->p >= mf->block_size*mf->block_num)
         {
            if(memfile_extend(mf, 5)<0)
            {
               fclose(fd);
               return 1;
            }
         }
         mf->data[(mf->p)++]=0;
      }

   }

   fclose(fd);

   return 0;
}


int memfile_printf(struct memfile_s *mf, char const* fmt, ...)
{
   va_list args;
   int ret;


   for(;;)
   {
      size_t n=(mf->block_size*mf->block_num)-mf->p;
      int ret=-1;

      va_start(args, fmt);
      ret=vsnprintf((char *)&(mf->data[mf->p]), n, fmt, args);
      va_end(args);

      if(ret<0)
         return ret;

      if(ret<n)
         break;
      else
      {
         if(memfile_extend(mf, 5)<0)
         {
            mf->p=(mf->block_size*mf->block_num)-1;
            return 1;
         }
      }
   }

   for(;mf->data[++(mf->p)];);

   return 0;
}

#ifdef TESTMODULE
int main(int argc, char *argv[])
{
   struct memfile_s *mf=memfile_alloc();

   memfile_init(mf, AUTOEXTEND, 10, 5);

   memfile_printf(mf, "TEST1\n"); 
   printf("block_num=%d p=%d\n",mf->block_num,mf->p);
   memfile_printf(mf, "TEST2\n"); 
   printf("block_num=%d p=%d\n",mf->block_num,mf->p);
   printf("%s\n",mf->data);

   memfile_release(mf);

   free(mf);
   mf=NULL;
}
#endif
