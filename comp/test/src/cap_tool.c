/******************************************************************************
* cap_tool.c
*
* author: dinglixing
* 
* 2012.05.31 create
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/******************************************************************************
* Marco & Const
******************************************************************************/

/******************************************************************************
* types
******************************************************************************/

typedef struct _cap_hdr_t {
    char magic  [4];
    char version[8];
    char time   [4];
    char xxxx   [112];
}cap_hdr_t;

typedef struct _cap_pkt_info_t {
    unsigned int   time;
    char xxxx   [4];
    unsigned short pkt_size;
    unsigned short pkt_size2;
    char yyyy   [28];
}cap_pkt_info_t;

/******************************************************************************
* function
******************************************************************************/
int cap_open(const char *name)
{
    int        fd, ret;
    cap_hdr_t  hdr;

    fd = open(name, O_RDONLY);
    if(0 > fd){
        return -1;
    }
    
    ret = read(fd, &hdr, sizeof(hdr));
    if(ret != sizeof(hdr)){
        close(fd);
        return -1;
    }

    if(strcmp("XCP", hdr.magic) != 0){
        close(fd);
        return -1;
    }

    return fd;
}

int cap_pkt_read(int cap, void *buf, int size)
{
    int             ret;
    cap_pkt_info_t  info;
    
    ret = read(cap, &info, sizeof(info));
    if(ret != sizeof(info)){
        if(ret == 0)
            return 0;
        else
            return -1;
    }

    if(info.pkt_size != info.pkt_size2){
        return -1;
    }

    if(size < info.pkt_size){
        return -1;
    }

    ret = read(cap, buf, info.pkt_size);
    if(ret != info.pkt_size){
        if(ret == 0)
            return 0;
        else
            return -1;
    }
    
    return ret;
}

int cap_close(int cap)
{
    return close(cap);
}

#if 0
static char buf[2048];
int main(int argc, char **argv)
{
    int cap, cnt = 0;

    
    if(argc != 2){
        printf("Usage: %s capfile\n", argv[0]);
        exit(0);
    }

    cap = cap_open(argv[1]);
    if(cap < 0){
        printf("cap_open: fail\n");
        exit(-1);
    }

    while(1){
        int len;
        
        len = cap_pkt_read(cap, buf, 2048);
        if(len == 0){
            break;
        }
        if(len < 0){
            printf("cap_pkt_read fail\n");
            break;
        }

        

        printf("%.8d: %d bytes\n", cnt++, len);
    }

    return cap_close(cap);
}
#endif

/* end of file */

