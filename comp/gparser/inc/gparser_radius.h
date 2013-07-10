/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gparser_radius.h
 *
 * Brief:
 *
 *
 * History:
 *
 *****************************************************************************/

#ifndef _GPARSER_RADIUS_INC_
#define _GPARSER_RADIUS_INC_

#include <gparser_pkt.h>

#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
#define GRADIUS_AVP_T               0
#define GRADIUS_AVP_L               1
#define GRADIUS_AVP_V               2

#define GRADIUS_CODE_OFF            0
#define GRADIUS_ID_OFF              1
#define GRADIUS_LEN_HI_OFF          2
#define GRADIUS_LEN_LO_OFF          3
#define GRADIUS_AUTH_OFF            4
#define GRADIUS_AVP_OFF             20

#define GRADIUS_OCT(r, off)         (*((uint8_t *)(r) + off))

#define GRADIUS_CODE(r)             GRADIUS_OCT(r, 0)
#define GRADIUS_ID(r)               GRADIUS_OCT(r, 1)
#define GRADIUS_LEN(r)              (GRADIUS_OCT(r, 2) << 8) + GRADIUS_OCT(r, 3)
#define GRADIUS_AVP(r)              ((uint8_t *)(r) + 20)

#define GRADIUS_HDR_SIZE            20

#define GRADIUS_ATTR_USERNAME       1
#define GRADIUS_ATTR_USERPW         2
#define GRADIUS_ATTR_CHAPPW         3
#define GRADIUS_ATTR_NASIPADDR      4

#define GRADIUS_ATTR_FRAMEPROTOCOL  7
#define GRADIUS_ATTR_FRAMEIPADDR    8
#define GRADIUS_ATTR_FRAMEIPMASK    9
#define GRADIUS_ATTR_FRAMEROUTE     10

#define GRADIUS_ATTR_ACCT_STATUS    40  /* acct status type */
#define GRADIUS_ATTR_ACCT_SID       44  /* acct session id  */

#define RADIUS_CODE_ACCS_REQUEST    1
#define RADIUS_CODE_ACCS_ACCEPT     2
#define RADIUS_CODE_ACCS_REJECT     3

#define RADIUS_CODE_ACCT_REQUEST    4
#define RADIUS_CODE_ACCT_RESPONSE   5

#define RADIUS_CODE_ACCS_CHALLENGE  11
#define RADIUS_CODE_STATUS_SVR      12
#define RADIUS_CODE_STATUS_CLI      13

#define GRADIUS_AVP_MAX             64

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/
typedef struct _gradius_info_t {
    uint8_t     *hdr;
    uint32_t     code;
    uint64_t     avp_bitmap;
    uint8_t     *avp[GRADIUS_AVP_MAX];
}gradius_info_t;

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int gparse_radius_parse(gpacket_info_t *pkt_info, gradius_info_t *radius_info);

static inline uint8_t *gparse_radius_hdr(gradius_info_t *radius_info)
{
    return radius_info->hdr;
}

static inline size_t   gparse_radius_hdr_cpy(void *buf, size_t len, gradius_info_t *radius_info)
{
    if(len < GRADIUS_HDR_SIZE){
        return 0;
    }

    memcpy(buf, radius_info->hdr, GRADIUS_HDR_SIZE);

    return GRADIUS_HDR_SIZE;
}

static inline uint8_t *gparse_radius_attr(gradius_info_t *radius_info, int attr)
{
    if((radius_info->avp_bitmap & (1 << attr)) && (NULL !=radius_info->avp[attr])){

        return radius_info->avp[attr];
    }else{
        return NULL;
    }
}

static inline size_t gparse_radius_attr_l(gradius_info_t *radius_info, int attr)
{
    if((radius_info->avp_bitmap & (1 << attr)) && (NULL !=radius_info->avp[attr])){

        return radius_info->avp[attr][GRADIUS_AVP_L] - 2;
    }else{
        return 0;
    }
}

static inline uint8_t *gparse_radius_attr_v(gradius_info_t *radius_info, int attr)
{
    if((radius_info->avp_bitmap & (1 << attr)) && (NULL !=radius_info->avp[attr]) ){
        return &radius_info->avp[attr][GRADIUS_AVP_V];
    }else{
        return NULL;
    }
}

static inline size_t gparse_radius_attr_cpy(void *buf, size_t len, gradius_info_t *radius_info, int attr)
{
    if((radius_info->avp_bitmap & (1 << attr)) && (NULL !=radius_info->avp[attr]) ){
        size_t attr_len = radius_info->avp[attr][GRADIUS_AVP_L] - 2;
        void  *attr_buf = &radius_info->avp[attr][GRADIUS_AVP_V];

        if(attr_len > len || attr_buf == NULL){
            return 0;
        }else{
            memcpy(buf, attr_buf, attr_len);
        }
        return attr_len;
    }else{
        return 0;
    }
}



int rediust_l_zero_get();


#ifdef _cplusplus
}
#endif

#endif /* _GPARSER_RADIUS_INC_ */

/* End of gparser_http.h */

