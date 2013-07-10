/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      config.c
 *
 * Brief:
 *      config file read
 *
 * History:
 *      12/06/2012
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/

#define VAL_INT      1
#define VAL_STRING  2

#define KEY_NUM   50
#define CMD_LEN   100

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/


typedef struct _cfg_hnd_t {
    FILE *fp;
    int  keyNum;
    int  keylen[KEY_NUM];
    int  vlen[KEY_NUM];
    char key[KEY_NUM][CMD_LEN];
    char value[KEY_NUM][CMD_LEN];;
}cfg_hnd;

typedef struct _cfg_line_info_t {
    char *line;
    int   key_idx;
    int   key_len;
    int   val_idx;
    int   val_len;
}cfg_line_info_t;


/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/

/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/

/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
static int cfg_line_parse(cfg_hnd *pCfg)
{
    int i = 0,j = 0;
    int linNum = 0;
    char cur = 0;
    int gc = 0;
    char line[1024] = {0};
    int  equLoc = 0;  /* "=" location */

    if( pCfg == NULL)
    {
        return 0;
    }

    for(i =0;i<KEY_NUM;)
    {
        j = 0;
        equLoc = 0;
        /* get line  tag '\r' or '\n', parse line */
        while(1)
        {
            gc = getc(pCfg->fp);
            if( gc == EOF) /* read end*/
            {
               break;
            }
            else
            {
                cur = (char)gc;
            }
            /*printf("%c ",cur);   */
            if( (cur == '\r')||(cur== '\n'))
            {
                 /* getline */
                 break;
            }
            else
            {
                if( (cur == '=')&&(equLoc== 0))
                {
                    equLoc = j;
                }
                line[j] = cur;
                j++;
                /* read max line break;*/
                if(  j>= CMD_LEN)
                {
                    break;
                }
            }
        }
        /* get key and value*/
        if( (j>1)&&(equLoc != 0)&&(line[0] != '#'))
        {
            int curk = 0;
            int curv = 0;
            linNum   = 0;
            /* get key */
            for(;linNum<equLoc;linNum++)
            {
                if((line[linNum]!=' ')&&(line[linNum]!='\t'))
                {
                    pCfg->key[i][curk] = line[linNum];
                    curk++;
                }
            }
            /* get v*/
            for(;linNum<j;linNum++)
            {
                if((line[linNum]!=' ')&&(line[linNum]!='='))
                {
                    pCfg->value[i][curv]=line[linNum];
                    curv++;
                }
            }
            /* valid key,value*/
            if((curk !=0)&&(curv !=0))
            {
                pCfg->keylen[i] = curk;
                pCfg->vlen[i] = curv;
                i++;
            }
        }
        /* end ?*/
        if( gc == EOF)
        {
            goto end;
        }
    }

end:
    pCfg->keyNum = i;
    return i;
}

void *cfg_open(char *cfg_file)
{
    cfg_hnd *cfg;
    void*    fp = NULL;

    fp = fopen(cfg_file,"r");

    if( fp == NULL)
    {
        return NULL;
    }

    cfg = (cfg_hnd*)malloc(sizeof(cfg_hnd));
    if( cfg == NULL)
    {
        fclose(fp);
        return NULL;
    }

    memset(cfg,0,sizeof(cfg));

    cfg->fp = fp;

    cfg_line_parse(cfg);

    return cfg;
}

void *cfg_wopen(char *cfg_file)
{
    cfg_hnd *cfg;
    void*    fp = NULL;

    fp = fopen(cfg_file,"w");

    if( fp == NULL)
    {

        return NULL;
    }

    cfg = (cfg_hnd*)malloc(sizeof(cfg_hnd));
    if( cfg == NULL)
    {
        fclose(fp);
        return NULL;
    }

    memset(cfg,0,sizeof(cfg));

    cfg->fp = fp;

    cfg_line_parse(cfg);

    return cfg;
}
/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/



int cfg_get_value(char *val,int vlen, int type, void *buf, int *size)
{
    int len = 0;
    switch(type){
        case VAL_STRING:
            len = (*size> CMD_LEN )?CMD_LEN:*size;
            len = (len>vlen)?vlen:len;
            memcpy(buf,val,len);
            break;
        case VAL_INT:
            if( *size !=4)
            {
                return -1;
            }
            else
            {
                int *p;
                p  = (int *)buf;
                *p = atoi(val);
            }
            break;
        default:
            return -1;

    }
    return 0;

}

int cfg_match(void *vcfg, char *key, char keylen,int val_type, void *val_buf, int *val_size)
{
    int i;
    cfg_hnd *cfg;

    cfg = vcfg;

    if( keylen> CMD_LEN)
    {
        return -2;
    }

    for(i = 0; i < KEY_NUM; i++){
        if(memcmp(key, cfg->key[i],keylen) == 0){
            return cfg_get_value(cfg->value[i],cfg->vlen[i], val_type, val_buf, val_size);
        }
    }

    return -1;
}

int cfg_close(void * vcfg)
{
    cfg_hnd *cfg = vcfg;

    fclose(cfg->fp);
    free(cfg);
    return 0;
}


int test_read_config(void)
{
    int           cfg_yes;
    char          cfg_name[128];
    int           v = 0;
    cfg_hnd *cfg;

    cfg = cfg_open("/mnt/hgfs/product/sgn_config.conf");
    if( cfg == NULL)
    {
        return 0;
    }
    v  = sizeof(cfg_name);
    cfg_match(cfg, "ip",sizeof("ip"),VAL_STRING,cfg_name,&v);
    v  = 4;
    cfg_match(cfg, "port",sizeof("port"),VAL_INT,&cfg_yes,&v);
    cfg_close(cfg);
    printf("get:ip;%s,get port:%d\r\n",cfg_name,cfg_yes);
    return 0;
}
