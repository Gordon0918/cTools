#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "cmdcfg.h"

#define MAXLOG (1024*100)
#define MAXPARAM 16

static char g_cmdcfgbuff[MAXLOG];
static int g_cmdcfgbuflen;
static int g_cmdcfglinepos=0;
char g_cmdcfgprefix[1024];
#define GCONFIG_PATH "GCONFIG_PATH"


char *get_cfgfullname(char *filename)
{
    (void)snprintf(g_cmdcfgprefix, sizeof(g_cmdcfgprefix), "%s/%s",
        getenv(GCONFIG_PATH),filename);
    return (char *)g_cmdcfgprefix;
}

char *get_cfgdirname()
{
    return getenv(GCONFIG_PATH);
}

void writelog(int level, char* info)
{
    printf("%s", info);
}

static int strsplit(char* str, char* split, char** childs, int maxnum)
{
    char* ptrbuf;
    char* child;
    int index = 0;
    while((child = strtok_r(str, split, &ptrbuf)) != NULL)
    {
        str = NULL;
        if(index == maxnum)
            return -1;
        childs[ index++ ] = child;

    }
    return index;
}

int cmdcfg_init(char* fname)
{
    int flen;
    int fd ;
    fd = open(fname, O_RDONLY|O_CREAT);
    if(fd < 0 )
    {
        perror("file open");
        return -1;
    }
    flen = read( fd, g_cmdcfgbuff, MAXLOG );
    close(fd);
    if(flen ==  MAXLOG )
    {
        writelog(0, "file size larged than  var filedata");
        return -1;
    }

    g_cmdcfgbuflen = strlen(g_cmdcfgbuff);
    g_cmdcfglinepos = 0;
    return 0;

}

char** cmdcfg_getnext(int *argc)
{
    char* line;
    static char linebuff[MAXLINE];
    static char* argv[MAXPARAM +1 ];

    if(g_cmdcfglinepos >= g_cmdcfgbuflen)
        return NULL;

    memset(argv, 0, sizeof(argv));
    while(g_cmdcfgbuff[g_cmdcfglinepos] == '\n')
    {
        g_cmdcfglinepos++;
        if(g_cmdcfglinepos == g_cmdcfgbuflen)
            return NULL;
    }

    line = &g_cmdcfgbuff[g_cmdcfglinepos];
    while(g_cmdcfgbuff[g_cmdcfglinepos] != '\n')
    {
        g_cmdcfglinepos++;
        if(g_cmdcfglinepos == g_cmdcfgbuflen)
            break;
    }
    g_cmdcfgbuff[g_cmdcfglinepos] = 0;
    if(strlen(line) >=  MAXLINE)
    {
        writelog(0, "line buff over size");
        return NULL;
    }
    strcpy(linebuff, line);
    g_cmdcfgbuff[g_cmdcfglinepos] = '\n';
    if( (*argc = strsplit( linebuff ," \t", argv, MAXPARAM))>0)
        return argv;
    return NULL;

}

int cmdcfg_add(int argc, char** argv)
{
    int i;
    int len = 0;
    for(i = 0; i < argc; i++)
    {
        len += strlen(argv[i]) +1;
    }
    if((len + g_cmdcfgbuflen) >= MAXLOG)
    {
        writelog(0, "no space for save log data");
        return -1;
    }
    for(i = 0; i < argc; i++)
    {
        strcpy( &g_cmdcfgbuff[g_cmdcfgbuflen], argv[i]);
        g_cmdcfgbuflen += strlen(argv[i]);
        g_cmdcfgbuff[g_cmdcfgbuflen]= ' ';
        g_cmdcfgbuflen ++;
    }
    g_cmdcfgbuff[g_cmdcfgbuflen -1]= '\n';
    return 0;

}
int cmdcfg_save(char* fname)
{
    int flen;
    int fd ;
    fd = open(fname, O_WRONLY|O_TRUNC);
    if(fd < 0 )
    {
        writelog(0, "file open error");
        return -1;
    }
    flen = write( fd, g_cmdcfgbuff, g_cmdcfgbuflen );
    close(fd);
    if(flen !=  g_cmdcfgbuflen )
    {
        writelog(0, "file write error");
        return -1;
    }

#ifdef HW_XLR732
    system("/root/save.sh");
#endif

    return 0;
}

#if 0
int main(int argc, char** argv)
{
    char** param;
    int i;
    char cmd[128];
    char lastcmd[128];
    int count;
    char* childcmd[16];
    while(1)
    {
        fgets(cmd, 128, stdin);
        printf("%s", cmd);
        if(memcmp(cmd, "quit", 4) == 0)
            break;

        if(cmd[0] == '\n')
        {
            strcpy(cmd, lastcmd);
            printf("%s",cmd);
        }
        else
            strcpy(lastcmd, cmd);

        if(memcmp(cmd, "load", 4) ==0)
        {
            if(cmdcfg_init("./test.log"))
            {
                break ;
            }
            continue;
        }
        if(memcmp(cmd, "next", 4) ==0)
        {
            param = cmdcfg_getnext();
            if(param != NULL)
            {
                i = 0;
                while(param[i])
                {
                    printf("%s ",param[i]);
                    i++;
                }
                printf("\n");
            }
            else
                printf("cmd if empty!!!!!!!!!!!!\n");
            continue;
        }
        if( memcmp(cmd, "add",3) == 0)
        {
            i = strsplit(cmd, " \t", (char**)&childcmd, 16);
            cmdcfg_add(i-1, &childcmd[1]);
            continue;
        }
        if( memcmp(cmd, "save",4) == 0)
        {
            cmdcfg_save("./test.log");
            continue;
        }

    }

}
#endif



