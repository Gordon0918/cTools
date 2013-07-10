
#ifndef __PUBLIC_H__
#define __PUBLIC_H__

#include <time.h>
#include <stdlib.h>
#include <string.h>



typedef unsigned int TIP;
typedef unsigned short ushort;
typedef unsigned char uchar;

#define MSGBUF  (2048)

#define galign(value, align) \
    (((value)+(align)) & (~(align -1)))

#define CHECK_MEMORY

#ifndef CHECK_MEMORY
#define MALLOC malloc
#define FREE   free
#define STRDUP strdup
#else
extern void *MALLOC(int size);
extern void FREE(void *p);
extern char *STRDUP(char *s);
#endif

extern time_t g_lbs_time;

static inline time_t get_curtime()
{
    return g_lbs_time;
}


#endif /*__PUBLIC_H__*/




