#ifndef __mea_memfile_h
#define __mea_memfile_h

#include <inttypes.h>

enum mea_mf_type_e { FIXE, AUTOEXTEND };
struct mea_memfile_s
{
   enum mea_mf_type_e type;
   uint32_t block_size;
   uint32_t block_num;
   
   char *data;

   uint32_t p;   
};

struct mea_memfile_s *mea_memfile_alloc();
struct mea_memfile_s *mea_memfile_init(struct mea_memfile_s *mf, enum mea_mf_type_e type, uint32_t bs, uint32_t nb);
int mea_memfile_release(struct mea_memfile_s *mf);
int mea_memfile_seek(struct mea_memfile_s *mf, uint32_t p);
int mea_memfile_extend(struct mea_memfile_s *mf, uint32_t nb);
int mea_memfile_printf(struct mea_memfile_s *mf, char const* fmt, ...);
int mea_memfile_include(struct mea_memfile_s *mf, char *file, int zero_ended);

#endif
