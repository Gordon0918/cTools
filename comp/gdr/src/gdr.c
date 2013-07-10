/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gdr.c
 *
 * Brief:
 *      geo data record module source file
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

#include <gdr.h>

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

/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
static inline int gdr_tlv_append_simple(void *tlv, gdr_len_t exlen, void *exval)
{
    uint8_t  *hdr  = (uint8_t*)tlv;
    gdr_len_t len = gdr_tlv_len(tlv);

    if(exval && exlen){
        memcpy((char *)tlv + GDR_VAL_OFFSET + len, exval, exlen);
    }

    len += exlen;
    hdr[GDR_LEN_HI] = (len >> 8) & 0xff;
    hdr[GDR_LEN_LO] = len & 0xff;

    return len + GDR_VAL_OFFSET;
}

static int gdr_tlv_append_complex(void *tlv, gdr_len_t append_len, void *append_buf)
{
    gdr_tag_t tag = gdr_tlv_tag(tlv);
    gdr_len_t len = gdr_tlv_len(tlv);

    if(   GDR_TLV_TYPE_SEQ != (tag & GDR_TLV_TYPE_MASK)
       || len == 0){
        return gdr_tlv_append_simple(tlv, append_len, append_buf);
    }else{
        void     *stlv = gdr_tlv_val(tlv);

        gdr_len_t off = 0;
        gdr_len_t stlv_size = gdr_tlv_size(stlv);
        //Goto last sub tlv
        while(off + stlv_size < len){
            off  += stlv_size;
            stlv  = (char *)stlv + stlv_size;

            stlv_size = gdr_tlv_size(stlv);
        }
        assert(off + stlv_size == len);

        gdr_tlv_append_complex(stlv, append_len, append_buf);
        return gdr_tlv_append_simple(tlv, append_len, NULL);
    }
}

static int _gdr_tlv_traverse(void *container, int level, void *tlv, int size, gdr_scan_hnd func, void *arg)
{
    int   offset = 0;
    int   scan_stat = 0;

    while(offset < size){
        void *cur_tlv = (char *)tlv + offset;
        int   cur_siz = gdr_tlv_size(cur_tlv);

        if(cur_siz > (size - offset)){
            printf("the invalid gdr packet!!!\n");
            //assert(0);
            //invalid gdr packet!!!
            scan_stat = 1;
            break;
        }

        scan_stat = func(arg, level, cur_tlv, container);

        if (scan_stat == 0 && GDR_TLV_TYPE_SEQ == (gdr_tlv_tag(cur_tlv) & GDR_TLV_TYPE_MASK)) {
            scan_stat = _gdr_tlv_traverse(cur_tlv, level + 1,
                                          gdr_tlv_val(cur_tlv),
                                          gdr_tlv_len(cur_tlv),
                                          func, arg);
        }
        offset += cur_siz;
        if(scan_stat != 0){
            break;
        }
    }

    return scan_stat;
}

/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/
void gdr_tlv_scan(void *container, void *tlv, int size, gdr_scan_hnd func, void *arg)
{
    assert(tlv && func);

    _gdr_tlv_traverse(container, 0, tlv, size, func, arg);
}

int gdr_tlv_append(void *tlv, int space, gdr_len_t append_len, void *append_buf)
{
    if(space < append_len){
        return -1;
    }

    return gdr_tlv_append_complex(tlv, append_len, append_buf);
}

/* End of gdr.c */

