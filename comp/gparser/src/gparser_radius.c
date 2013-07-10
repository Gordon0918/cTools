/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gparser_radius.c
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
#include <gparser_radius.h>

/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/


/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/


/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/


/******************************************************************************
 * Local  variable define                                                     *
 *****************************************************************************/


/******************************************************************************
 * Global variable define                                                     *
 *****************************************************************************/
static int rediust_l_zero;


int rediust_l_zero_get()
{
    return rediust_l_zero;
}

/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
static void radius_parse_attr(uint8_t *avp,
                              size_t   avp_len,
                              gradius_info_t *radius_info)
{
    size_t  off = 0;
    size_t  lft = avp_len;

    while(off < avp_len){
        uint8_t t, l;
        uint8_t *cur;

        cur  = avp + off;
        t    = cur[0];
        l    = cur[1];

        if(l < 2 || lft < l){
            radius_info->avp_bitmap = 0;
            break;
        }

        off += l;
        lft -= l;

        if(t < GRADIUS_AVP_MAX){
            radius_info->avp_bitmap |= 1 << t;
            radius_info->avp[t]      = cur;
        }
        #if 0
        else{
            //same error!!
            if(t >= GRADIUS_AVP_MAX) {
                rediust_l_zero++;
                continue;
            }
            radius_info->avp_bitmap = 0;
            break;
        }
        #endif
    }
}

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
int gparse_radius_parse(gpacket_info_t *pkt_info, gradius_info_t *radius_info)
{
    uint32_t     code;
    uint8_t     *radius;
    uint8_t     *avp;
    size_t       avp_len;

    radius = gparse_pkt_payload(pkt_info);

    code   = GRADIUS_CODE(radius);
    //We care accounting request only!
    if(code != RADIUS_CODE_ACCT_REQUEST){
        return -1;
    }

    radius_info->hdr        = radius;
    radius_info->code       = code;
    radius_info->avp_bitmap = 0;

    avp     = GRADIUS_AVP(radius);
    avp_len = GRADIUS_LEN(radius) - GRADIUS_HDR_SIZE;
    radius_parse_attr(avp, avp_len, radius_info);

    if(radius_info->avp_bitmap == 0) {
        return -2;
    }

    return 0;
}

/* End of gparser_http.c */
