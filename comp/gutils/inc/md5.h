
#ifndef MY_MD5_H
#define MY_MD5_H 1

#include <stdio.h>
#define MD5_DIGEST_SIZE 16
#define MD5_BLOCK_SIZE 64

#ifdef _LIBC
typedef uint32_t md5_uint32;
typedef uintptr_t md5_uintptr;
#else
# if defined __STDC__ && __STDC__
#  define UINT_MAX_32_BITS 4294967295U
# else
#  define UINT_MAX_32_BITS 0xFFFFFFFF
# endif

# ifndef UINT_MAX
#  define UINT_MAX UINT_MAX_32_BITS
# endif

# if UINT_MAX == UINT_MAX_32_BITS
   typedef unsigned int md5_uint32;
# else
#  if USHRT_MAX == UINT_MAX_32_BITS
    typedef unsigned short md5_uint32;
#  else
#   if ULONG_MAX == UINT_MAX_32_BITS
     typedef unsigned long md5_uint32;
#   else
     "Cannot determine unsigned 32-bit data type."
#   endif
#  endif
# endif
typedef unsigned long int md5_uintptr;
#endif

struct md5_ctx
{
  md5_uint32 A;
  md5_uint32 B;
  md5_uint32 C;
  md5_uint32 D;

  md5_uint32 total[2];
  md5_uint32 buflen;
  char buffer[128] __attribute__ ((__aligned__ (__alignof__ (md5_uint32))));
};

#ifdef __THROW
#define __THROW_TMP __THROW
#undef __THROW
#define __THROW
#endif
extern void md5_init_ctx (struct md5_ctx *ctx) __THROW;

extern void md5_process_block (const void *buffer, size_t len,
                 struct md5_ctx *ctx) __THROW;

extern void md5_process_bytes (const void *buffer, size_t len,
                 struct md5_ctx *ctx) __THROW;

extern void *md5_finish_ctx (struct md5_ctx *ctx, void *resbuf) __THROW;

extern void *md5_read_ctx (const struct md5_ctx *ctx, void *resbuf) __THROW;

extern int md5_string(const void *data, size_t len, char string[32]) __THROW;

extern int md5_binary(const void *data, size_t len, unsigned char binary[16]) __THROW;

extern void *md5_read_ctx_str (const struct md5_ctx *ctx, void *resbuf, void *buf) __THROW;

extern void *md5_finish_ctx_str (struct md5_ctx *ctx, void *resbuf, void *buf) __THROW;

#undef __THROW

#define __THROW __THROW_TMP

#endif



