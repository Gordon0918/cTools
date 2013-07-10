/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gparser_http.h
 *
 * Brief:
 *
 *
 * History:
 *
 *****************************************************************************/

#ifndef _GPARSER_HTTP_INC_
#define _GPARSER_HTTP_INC_

#include"gparser_pkt.h"

#ifdef _cplusplus
extern "C" {
#endif


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GHTTP_PW_MAX        16      /* privcy word max*/
#define GHTTP_KW_MAX        8       /* key word max */
#define GHTTP_TOK_MAX        32        /* ?? */


enum gparse_http_attr_e {
    GHTTP_ATTR_HOST         = 0,
    GHTTP_ATTR_REFER        = 1,
    GHTTP_ATTR_UA           = 2,
    GHTTP_ATTR_COOKIE       = 3,
    GHTTP_ATTR_FORMDATA     = 4,
    GHTTP_ATTR_KWORD        = 5,
    GHTTP_ATTR_MAX          = 6,
    GHTTP_URL               = 7,
    GHTTP_ATTR_IGN          = 8,
};

enum gparse_http_method_e {
    GHTTP_METHOD_UNKNOWN    = 0,
    GHTTP_METHOD_GET        = 1,
    GHTTP_METHOD_POST       = 2,
    GHTTP_METHOD_HEAD       = 3,
};


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _ghttp_region_t {
    size_t idx;
    size_t len;
    size_t prefix_len;
}ghttp_region_t;

typedef struct __ghttp_info_t {
    uint8_t         *data;

    size_t           dsize;
    uint8_t          method;
    uint8_t          reserve[3];

    uint32_t         attr_cur;
    uint32_t         attr_flag;
    ghttp_region_t   url;
    ghttp_region_t   attr[GHTTP_ATTR_MAX];
    char             host_url[2048];
    int              host_url_prefix_len;
    int              host_url_len;    //has the url arg
    void             *url_attr;
}ghttp_info_t;

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
extern int gparse_http_init();
extern int gparse_http_deinit();
extern int gparse_http_parse(gpacket_info_t *pkt_info,
                             ghttp_info_t *http_info);

static inline int    gparse_http_method(ghttp_info_t *http_info)
{
    return http_info->method;
}

static inline int    gparse_http_is_get(ghttp_info_t *http_info)
{
    return (http_info->method == GHTTP_METHOD_GET);
}

static inline int    gparse_http_is_head(ghttp_info_t *http_info)
{
    return (http_info->method == GHTTP_METHOD_HEAD);
}

static inline int    gparse_http_is_post(ghttp_info_t *http_info)
{
    return (http_info->method == GHTTP_METHOD_POST);
}

static inline size_t gparse_http_url_l(ghttp_info_t *http_info)
{
    return http_info->url.len;
}

static inline size_t gparse_http_url_prefix_l(ghttp_info_t *http_info)
{
    return http_info->url.prefix_len;
}

static inline size_t gparse_http_url_args_l(ghttp_info_t *http_info)
{
    return http_info->url.len - http_info->url.prefix_len;
}

static inline void * gparse_http_url(ghttp_info_t *http_info)
{
    return http_info->data + http_info->url.idx;
}

static inline void * gparse_http_url_prefix_e(ghttp_info_t *http_info)
{
    return http_info->data
         + http_info->url.idx
         + http_info->url.prefix_len;
}

static inline void * gparse_http_url_args(ghttp_info_t *http_info)
{
    return http_info->data
         + http_info->url.idx
         + http_info->url.prefix_len;
}

static inline size_t gparse_http_attr_l(ghttp_info_t *http_info, int attr)
{
    if(   attr < GHTTP_ATTR_MAX
       && (http_info->attr_flag & (1 << attr))){
        ghttp_region_t *attr_rgn = &http_info->attr[attr];

        return attr_rgn->len;
    }else{
        return 0;
    }
}

static inline size_t gparse_http_attr_content_l(ghttp_info_t *http_info, int attr)
{
    if(   attr < GHTTP_ATTR_MAX
       && (http_info->attr_flag & (1 << attr))){
        ghttp_region_t *attr_rgn = &http_info->attr[attr];

        return attr_rgn->len - attr_rgn->prefix_len;
    }else{
        return 0;
    }
}

static inline void * gparse_http_attr(ghttp_info_t *http_info, int attr)
{
    if(   attr < GHTTP_ATTR_MAX
       && (http_info->attr_flag & (1 << attr))){
        ghttp_region_t *attr_rgn = &http_info->attr[attr];

        return http_info->data + attr_rgn->idx;
    }else{
        return NULL;
    }
}

static inline void * gparse_http_attr_prefix(ghttp_info_t *http_info, int attr)
{
    if(   attr < GHTTP_ATTR_MAX
       && (http_info->attr_flag & (1 << attr))){
        ghttp_region_t *attr_rgn = &http_info->attr[attr];

        return http_info->data + attr_rgn->idx;
    }else{
        return NULL;
    }
}

static inline void * gparse_http_attr_content(ghttp_info_t *http_info, int attr)
{
    if(   attr < GHTTP_ATTR_MAX
       && (http_info->attr_flag & (1 << attr))){
        ghttp_region_t *attr_rgn = &http_info->attr[attr];

        return http_info->data + attr_rgn->idx + attr_rgn->prefix_len;
    }else{
        return NULL;
    }
}

static inline int  gparse_http_is_homepage(ghttp_info_t *http_info)
{
    return gparse_http_url_l(http_info) < 2;
}

static inline int  gparse_http_is_path(ghttp_info_t *http_info)
{
    int url_len = gparse_http_url_l(http_info);
    char *url  = gparse_http_url(http_info);

    if(url[url_len-1] == '/') {
        return 1;
    }else {
        return 0;
    }
}


static inline int gparse_http_has_refer(ghttp_info_t *http_info)
{
    return (gparse_http_attr_content(http_info, GHTTP_ATTR_REFER) != NULL);
}

static inline int gparse_http_get_complete(ghttp_info_t *http_info)
{
    return 0;
}


static inline int   http_host_refer_compare(ghttp_info_t *http_info)
{
    int  url_len =  gparse_http_url_l(http_info);
    int  host_len =  (int)gparse_http_attr_content_l(http_info, GHTTP_ATTR_HOST);
    int  refer_len = (int)gparse_http_attr_content_l(http_info,GHTTP_ATTR_REFER);

    if(refer_len <= 0 || host_len <= 0) {
        return -1;
    }

    char *host =     gparse_http_attr_content(http_info, GHTTP_ATTR_HOST);
    char *refer =    gparse_http_attr_content(http_info, GHTTP_ATTR_REFER);
    char *url   =    gparse_http_url(http_info);

    char host_url[2048];
    int  host_url_len;
    if(host != NULL && url != NULL && refer != NULL) {
      strcpy(host_url,"http://");
      strncat(host_url,host,host_len);
      strncat(host_url,url,url_len);
      host_url_len = strlen(host_url);
      if(refer_len != host_url_len) {
        return -1;
      }
      int maxlen = refer_len > host_url_len ? refer_len : host_url_len;
      int result = memcmp(host_url,refer,maxlen);
      return result;
    }

    return -1;
}






#ifdef _cplusplus
}
#endif

#endif /* _GPARSER_HTTP_INC_ */

/* End of gparser_http.h */

