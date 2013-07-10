/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gparser_http.c
 *
 * Brief:
 *
 *
 * History:
 *
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <assert.h>
#include <string.h>

#include "gparser_http.h"


//#include "config_file.h"

/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
enum ghttp_pat_typ_e {
    GHTTP_SPLT       = 0,
    GHTTP_TERM       = 1,
    GHTTP_ATTR       = 2,
    GHTTP_OVER       = 3,
    GHTTP_TOKEN      = 9,


    GHTTP_BSFX       = 3,
    GHTTP_WUA        = 4,
    GHTTP_KHOST      = 5,
    GHTTP_BHOST      = 6,
    GHTTP_KWORD      = 7,
    GHTTP_PWORD      = 8,
};

enum ghttp_pkw_e {
    GHTTP_FIND_KWORD  = 1,
    GHTTP_FIND_PWORD  = 2,
};



enum gparse_http_keyword {
    HTTP_HOST      = 0,
    HTTP_REFER     = 1,
    HTTP_UA        = 2,
    HTTP_COOKIE    = 3,
    HTTP_HTTP11    = 4,
    HTTP_HTTP10    = 5,
    HTTP_URL       = 5,   //eque to HTTP_HTTP10
    HTTP_RETURN    = 6,
    HTTP_SPT1      = 7,
    HTTP_SPT2      = 8,
    HTTP_SPT3      = 9,
    HTTP_MAXITEM   = 10,
};

enum gparse_http_host_type{
    HOST_TYPE_IGNORE    = 0,
    HOST_TYPE_INTEREST  = 1,
    HOST_TYPE_KEYWORD   = 2,
};


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
#if 0
#define HTTP_KW_MAX         2
#define HTTP_KW_SIZE        20


typedef struct _ghttp_tok_t {
    int  ploy;  //
    int  param;
}ghttp_tok_t;

typedef struct _ghttp_pat_t {
    int     pat_typ;
    size_t  pat_len;
    union {
        int attr_id;
        int is_urlend;
        ghttp_tok_t token;
    }param;
}ghttp_pat_t;


typedef struct _key_info_t{
    char kw[HTTP_KW_SIZE];
    int  kw_len;
}key_info_t;

typedef struct _host_info_t{
    int  host_type;
    int  kw_num;
    int  kw_ids[HTTP_KW_MAX];
}host_info_t;

#endif
/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/

/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/

/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
int gparse_http_init()
{
    return 0;
}

int gparse_http_deinit()
{
    return 0;
}

int gparse_http_parse(gpacket_info_t *pkt_info, ghttp_info_t *http_info)
{
    int    ret = -1;
    size_t dsize;
    char   ch;
    char   *data;
    int     pos = 0;

    data  = (char *)pkt_info->data;
    dsize = pkt_info->dsize;
#if 0
    if(dsize < 128){
        printf("dsize < 32\n");
        return -1;
    }
#endif

    http_info->attr_flag = 0;

    http_info->data  = (void *)data;
    http_info->dsize = dsize;

    if(memcmp("GET ", &data[pos], 4) == 0){        //kaolvdaxiaoxie
        pos += 4;
        http_info->method = GHTTP_METHOD_GET;
    }else if(memcmp("HEAD ", &data[pos], 5) == 0){
        pos += 5;
        http_info->method = GHTTP_METHOD_HEAD;
    }else if(memcmp("POST ", &data[pos], 5) == 0){
        pos += 5;
        http_info->method = GHTTP_METHOD_POST;
    }else{
        http_info->method = GHTTP_METHOD_UNKNOWN;
        data[5] = 0;
        //printf("GHTTP_METHOD_UNKNOWN : %s\n", data);
        return -1;
    }
    http_info->url.idx = pos;

//GHTTP_URL:
    while(pos < dsize){
        ch = data[pos++];
        if(ch == '?'){
            http_info->url.prefix_len = (pos - 1) - http_info->url.idx;
            break;
        }else if(ch == ' '){
            http_info->url.prefix_len = (pos - 1) - http_info->url.idx;
            http_info->url.len = http_info->url.prefix_len;
            goto GHTTP_URL_END;
        }
    }
//GHTTP_URL_ARG:
    while(pos < dsize){
        ch = data[pos++];
        if(ch == ' '){
            http_info->url.len = (pos - 1) - http_info->url.idx;
            break;
        }
    }
GHTTP_URL_END:
    while(pos < (dsize - 1)){
        ch = data[pos++];
        if(ch == '\r'){
            ch = data[pos++];
            if(ch == '\n'){
                ret = 0;
                goto GHTTP_ATTR_PARSE;
            }else{
                goto GHTTP_END;
            }
        }
    }

GHTTP_ATTR_PARSE:
    if(pos >= dsize - 12){
        goto GHTTP_END;
    }
    if(memcmp("Host: ", &data[pos], 6) == 0){
        http_info->attr_cur = GHTTP_ATTR_HOST;
        http_info->attr[GHTTP_ATTR_HOST].idx = pos;
        http_info->attr[GHTTP_ATTR_HOST].prefix_len = 6;
        pos += 6;
    }else if(memcmp("User-Agent: ", &data[pos], 12) == 0){
        http_info->attr_cur = GHTTP_ATTR_UA;
        http_info->attr[GHTTP_ATTR_UA].idx = pos;
        http_info->attr[GHTTP_ATTR_UA].prefix_len = 12;
        pos += 12;
    }else if(memcmp("Referer: ", &data[pos], 9) == 0){
        http_info->attr_cur = GHTTP_ATTR_REFER;
        http_info->attr[GHTTP_ATTR_REFER].idx = pos;
        http_info->attr[GHTTP_ATTR_REFER].prefix_len = 9;
        pos += 9;
    }else if(memcmp("Cookie: ", &data[pos], 8) == 0){
        http_info->attr_cur = GHTTP_ATTR_COOKIE;
        http_info->attr[GHTTP_ATTR_COOKIE].idx = pos;
        http_info->attr[GHTTP_ATTR_COOKIE].prefix_len = 8;
        pos += 8;
    }else if(memcmp("\r\n", &data[pos], 2) == 0){
        goto GHTTP_END;
    }else{
        http_info->attr_cur = GHTTP_ATTR_IGN;
    }
    while(pos < dsize - 1){
        ch = data[pos++];

        if(ch == '\r'){
            ch = data[pos++];
            if(ch == '\n'){
                if(GHTTP_ATTR_IGN != http_info->attr_cur){
                    http_info->attr_flag |= 1 << http_info->attr_cur;
                    http_info->attr[http_info->attr_cur].len =
                        pos - 2 - http_info->attr[http_info->attr_cur].idx;
                }
                goto GHTTP_ATTR_PARSE;
            }else{
                goto GHTTP_END;
            }
        }else if(ch == '\n'){
            ch = data[pos-2];
            if(ch == '\r'){
                if(GHTTP_ATTR_IGN != http_info->attr_cur){
                    http_info->attr_flag |= 1 << http_info->attr_cur;
                    http_info->attr[http_info->attr_cur].len =
                        pos - 2 - http_info->attr[http_info->attr_cur].idx;
                }
                goto GHTTP_ATTR_PARSE;
            }else{
                goto GHTTP_END;
            }
        }

        //jump 2 ch
        pos++;
    }

GHTTP_END:
    //get port data idx and len
    if(http_info->method == GHTTP_METHOD_POST) {
        int form_data_len = http_info->dsize-pos-2;
        if(memcmp("\r\n", &data[pos], 2) == 0 && form_data_len > 5) {
          http_info->attr[GHTTP_ATTR_FORMDATA].idx = pos + 2;
          http_info->attr[GHTTP_ATTR_FORMDATA].prefix_len = 0;
          http_info->attr[GHTTP_ATTR_FORMDATA].len = http_info->dsize - pos - 2;

          http_info->attr_flag |= 1 << GHTTP_ATTR_FORMDATA;
          //char tmp = http_info->data[http_info->dsize];
          //write(1,http_info->attr[GHTTP_ATTR_POST_DATA].idx + http_info->data,http_info->attr[GHTTP_ATTR_POST_DATA].len);
        }
    }
    return ret;
}

/* End of gparser_http.c */



