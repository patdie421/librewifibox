#ifndef __memfile_h
#define __memfile_h

#include <inttypes.h>

enum mf_type_e { FIXE, AUTOEXTEND };
struct memfile_s
{
   enum mf_type_e type;
   uint32_t block_size;
   uint32_t block_num;
   
   char *data;

   uint32_t p;   
};

struct memfile_s *memfile_alloc();
struct memfile_s *memfile_init(struct memfile_s *mf, enum mf_type_e type, uint32_t bs, uint32_t nb);
int memfile_release(struct memfile_s *mf);
int memfile_seek(struct memfile_s *mf, uint32_t p);
int memfile_extend(struct memfile_s *mf, uint32_t nb);
int memfile_printf(struct memfile_s *mf, char const* fmt, ...);
int memfile_include(struct memfile_s *mf, char *file, int zero_ended);

#endif
