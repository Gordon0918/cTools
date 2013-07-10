/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gdr.h
 *
 * Brief:
 *      geo data record module header file
 *
 * History:
 *      12/06/2012  create dinglixing
 *****************************************************************************/

#ifndef _GDR_INC_
#define _GDR_INC_

#include <string.h>
#include <stdint.h>


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GDR_SIZE_MAX            (2048)

#define GDR_LAYER_MAX           (10)
#define GDR_TAG_HI              0
#define GDR_TAG_LO              1
#define GDR_LEN_HI              2
#define GDR_LEN_LO              3
#define GDR_TAGLEN_SIZ          4
#define GDR_VAL_OFFSET          4

#define GDR_TLV_CLASS_MASK      0x8000
#define GDR_TLV_CLASS_GLOBAL    0x8000
#define GDR_TLV_CLASS_LOCAL     0x0000

#define GDR_TLV_TYPE_MASK       0x7000
#define GDR_TLV_TYPE_RSV        0x0000
#define GDR_TLV_TYPE_SEQ        0x1000
#define GDR_TLV_TYPE_NUM        0x2000
#define GDR_TLV_TYPE_UNUM       0x3000
#define GDR_TLV_TYPE_OCT_STR    0x4000

#define GDR_TLV_TAG_MASK        0x0fff

/* Http access table Tag */
#define GDR_TBL_HTTPACS         0x9080
/* Http Key Word table Tag */
#define GDR_TBL_HTTPKW          0x9090
#define GDR_TBL_HTTPWH          0x9091
#define GDR_TBL_HTTPSNS         0X9092


/* Http access & kw table attribute Tags */
#define GDR_ATTR_HTTP_TS             0x3001
#define GDR_ATTR_HTTP_RAT            0x3002
#define GDR_ATTR_HTTP_METHOD         0x3003
#define GDR_ATTR_HTTP_UID            0x4001
#define GDR_ATTR_HTTP_HOST           0x4002
#define GDR_ATTR_HTTP_URL            0x4003
#define GDR_ATTR_HTTP_UA             0x4004
#define GDR_ATTR_HTTP_REFER          0x4005
#define GDR_ATTR_HTTP_COOKIE         0x4006
#define GDR_ATTR_HTTP_KW             0x4007
#define GDR_ATTR_HTTP_FORMDATA       0x4008
#define GDR_ATTR_HTTP_COMPLETE_URL   0x4009

#define GDR_ATTR_HTTP_PKT            0x40FF
#define GDR_ATTR_HTTP_IP             0x400A
#define GDR_ATTR_HTTP_KEYSITE        0x400B

/* Radius table Tag */
#define GDR_TBL_RADIUS          0x90A0
/* Radius table attribute Tags */
#define GDR_TBL_DHCP            0x90A1
#define GDR_ATTR_RADIUS_EVENT   0x3004
#define GDR_ATTR_RADIUS_UID     0x4001
#define GDR_ATTR_RADIUS_UIP     0x400A
#define GDR_ATTR_RADIUS_NIP     0x4008
#define GDR_ATTR_RADIUS_SID     0x4009

#define GDR_ATTR_RADIUS_PLY     0x40E0
#define GDR_ATTR_RADIUS_PUSH    0x40E1

#define GDR_ATTR_RADIUS_PKT     0x40FF

/* Account Query table Tag */
#define GDR_TBL_ACCQ            0x90B0
/* Account Query attribute Tags */
#define GDR_ATTR_ACCQ_IP46      0x400A
#define GDR_ATTR_ACCQ_AREA      0x400C

/* 3GPP table Tag */
#define GDR_TBL_3GPP            0x90d0
/* 3GPP table attribute Tag */
#define GDR_ATTR_3GPP_EVENT                   0x3001

#define GDR_ATTR_3GPP_CAUSE                   0x4000
#define GDR_ATTR_3GPP_TEID_IE                 0x4002
#define GDR_ATTR_3GPP_TEID                    0x4003
#define GDR_ATTR_3GPP_END_USER_IP             0x4004
#define GDR_ATTR_3GPP_GSNC_IP                 0x4005
#define GDR_ATTR_3GPP_GSNU_IP                 0x4006
#define GDR_ATTR_3GPP_IMSI                    0x4007
#define GDR_ATTR_3GPP_NSAPI                   0x4008
#define GDR_ATTR_3GPP_LNSAPI                  0x4009
#define GDR_ATTR_3GPP_APN                     0x400a
#define GDR_ATTR_3GPP_USER_LOC_INFO           0x400b
#define GDR_ATTR_3GPP_CELL_NUM                0x400c
#define GDR_ATTR_3GPP_TEARDOWN                0x400d
#define GDR_ATTR_3GPP_DIRECT                  0x400e

/*3gpp direction sync event*/
#define GDR_TBL_3GDRCT            0x90e0

#define GDR_ATTR_3GDRCT_SGSNIP     0x4001
#define GDR_ATTR_3GDRCT_GGSNIP     0x40a0

/*notify*/
#define GDR_TBL_PUSHINFORM                    0x90C0
#define GDR_ATTR_PUSHINFORM_IP46              0x400A
#define GDR_ATTR_PUSHINFORM_UID               0x4001
#define GDR_ATTR_PUSHINFORM_MC                0x4002


#define GDR_TBL_REQIMG          0x90E0
#define GDR_TBL_DSPKA           0x90E1
#define GDR_TBL_IMINGKA         0x90e2

#define GDR_IMG_MSGID          0x4001
#define GDR_IMG_COOKIE         0x4002
#define GDR_IMG_IPADDR         0x4003
#define GDR_IMG_PORT           0x4004



#define GDR_DSP_USER_TABLE         0x90E3
#define GDR_DSP_USER_RTABLE        0x90C3
#define GDR_DSP_GROUP_TABLE        0x90E4
#define GDR_DSP_GROUP_RTABLE       0x90C4
#define GDR_DSP_KWORD_TABLE        0x90E5
#define GDR_DSP_KWORD_RTABLE       0x90C5
#define GDR_DSP_KHOST_TABLE        0x90E6
#define GDR_DSP_KHOST_RTABLE       0x90C6
#define GDR_DSP_GROUP_KWORD_TABLE  0x90E7
#define GDR_DSP_GROUP_KWORD_RTABLE 0x90C7
#define GDR_DSP_GROUP_KW_HW_TABLE  0x90E8
#define GDR_DSP_GROUP_KW_HW_RTABLE 0x90C8

#define GDR_ATTR_DSP_MESSAGEID  0x4001
#define GDR_ATTR_DSP_COOKIEID   0x4002
#define GDR_ATTR_DSP_STIP       0x4003      //station ip
#define GDR_ATTR_DSP_STPORT        0x4004      //station port
#define GDR_ATTR_DSP_URL        0x4005
#define GDR_ATTR_DSP_DISTRIBUTEID  0x2001
#define GDR_ATTR_DSP_RESULTNUM    0x2002
#define GDR_ATTR_DSP_GENDER        0x2003
#define GDR_ATTR_DSP_AGE         0x2004
#define GDR_ATTR_DSP_JOB        0x2005
#define GDR_ATTR_DSP_INCOME        0x2006
#define GDR_ATTR_DSP_BABY        0x2007
#define GDR_ATTR_DSP_MARRIAGE   0x2008
#define GDR_ATTR_DSP_EDUCATION    0x2009
#define GDR_ATTR_DSP_REGION        0x2010
#define GDR_ATTR_DSP_INTEREST   0x4006
#define GDR_ATTR_DSP_PRODUCT    0x4007
#define GDR_ATTR_DSP_UIP        0x400A      //userip
#define GDR_ATTR_DSP_UID        0x400B      //userport
#define GDR_ATTR_DSP_KWH_NUM    0x2011
#define GDR_ATTR_DSP_KWORD      0x4008
#define GDR_ATTR_DSP_KHOST      0x4009
#define GDR_ATTR_DSP_HWORD      0x4010

#define SYS_UID_LEN             64
#define GDR_UID_LEN             20
#define GDR_MSG_KA              0X9000

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef uint16_t gdr_tag_t;
typedef uint16_t gdr_len_t;

/*
 * gdr_proc_hnd
 *
 * param:
 *      user_arg
 *
 * return:
 *      0,     scan continue
 *      other, scan skip
 */
typedef int (*gdr_scan_hnd)(void *user_arg, int level, void *tlv, void *container);

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int  gdr_tlv_append(void *tlv, int space, gdr_len_t len, void *val);
void gdr_tlv_scan(void *container, void *tlv, int size, gdr_scan_hnd func, void *arg);

static inline int gdr_tlv_init(void *buf, gdr_tag_t tag)
{
    uint8_t *hdr = (uint8_t*)buf;

    hdr[GDR_TAG_HI] = (tag >> 8) & 0xff;
    hdr[GDR_TAG_LO] = tag & 0xff;
    hdr[GDR_LEN_HI] = 0;
    hdr[GDR_LEN_LO] = 0;

    return GDR_TAGLEN_SIZ;
}

static inline gdr_tag_t gdr_tlv_tag(void *tlv)
{
    uint8_t *hdr = (uint8_t*)tlv;

    return (hdr[GDR_TAG_HI] << 8) | hdr[GDR_TAG_LO];
}

static inline gdr_tag_t gdr_tlv_tag_set(void *tlv, gdr_tag_t tag)
{
    uint8_t *hdr = (uint8_t*)tlv;

    hdr[GDR_TAG_HI] = (tag >> 8) & 0xff;
    hdr[GDR_TAG_LO] = tag & 0xff;

    return tag;
}

static inline gdr_len_t gdr_tlv_len(void *tlv)
{
    uint8_t *hdr = (uint8_t*)tlv;

    return (hdr[GDR_LEN_HI] << 8) | hdr[GDR_LEN_LO];
}

static inline gdr_len_t gdr_tlv_len_set(void *tlv, gdr_len_t len)
{
    uint8_t *hdr = (uint8_t*)tlv;

    hdr[GDR_LEN_HI] = (len >> 8) & 0xff;
    hdr[GDR_LEN_LO] = len & 0xff;

    return len;
}

static inline char * gdr_tlv_val(void *tlv)
{
    return (char *)tlv + GDR_TAGLEN_SIZ;
}

static inline gdr_len_t gdr_tlv_size(void *tlv)
{
    uint8_t *hdr = (uint8_t*)tlv;

    return ((hdr[GDR_LEN_HI] << 8) | hdr[GDR_LEN_LO]) + GDR_TAGLEN_SIZ;
}

static inline int gdr_tlv_add(void *container, int space, gdr_tag_t tag, gdr_len_t len, void *val)
{
    gdr_len_t container_len;
    void *stlv;

    if(space < GDR_TAGLEN_SIZ + len){
        return -1;
    }
    container_len = gdr_tlv_len(container);
    stlv = (char *)container + GDR_TAGLEN_SIZ + container_len;

    gdr_tlv_tag_set(stlv, tag);
    gdr_tlv_len_set(stlv, len);
    if(val){
        memcpy(gdr_tlv_val(stlv), val, len);
    }

    //container_size
    container_len += GDR_TAGLEN_SIZ + len;
    gdr_tlv_len_set(container, container_len);

    return container_len + GDR_TAGLEN_SIZ;
}


#endif /* _GDR_INC_ */

/* End of gdr.h */

