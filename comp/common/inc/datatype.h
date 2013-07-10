#ifndef __DATATYPE__H
#define __DATATYPE__H

#if defined(_WIN32) || defined(WIN32)
#define __INLINE__
#else    /* WIN32 */
#define __INLINE__ inline
#endif    /*!WIN32 */

typedef    unsigned char        uint08;
typedef    unsigned short        uint16;
typedef    unsigned int        uint32;

#if defined(_WIN32) || defined(WIN32)
typedef __int64                uint64;
#else    /* WIN32 */
typedef    unsigned long long    uint64;
#endif    /*!WIN32 */

typedef    signed char            int08;
typedef    signed short        int16;
typedef    signed int            int32;

#if defined(_WIN32) || defined(WIN32)
typedef __int64                int64;
#else    /* WIN32 */
typedef    signed long long    int64;
#endif    /*!WIN32 */

//typedef    char                string;

#define    BIT_PER_UINT32        32
#define BIT_PER_INT32        32

#define MIN(a, b)            ((a) < (b) ? (a) : (b))
#define MAX(a, b)            ((a) > (b) ? (a) : (b))

#define ARRSIZE(a)            (sizeof((a))/sizeof((a)[0]))

#define  addr_align(p, n)     ((unsigned long)((unsigned long)(p) + (n) - 1) & (~((n) - 1)))

/* convert from member's pointer to struct/class's pointer */
#define mem2ptr(type, mm, mptr)        ((type *)((uint08 *)mptr - (unsigned long)&((type *)0)->mm))

#if defined(_WIN32) || defined(WIN32)
#define strncasecmp    _strnicmp
#define strcasecmp    _stricmp
#define snprintf    _snprintf
#define vsnprintf    _vsnprintf
#define fstat        _fstat
#define fileno        _fileno
#endif /* WIN32 */

#endif    /* __DATATYPE__H */
