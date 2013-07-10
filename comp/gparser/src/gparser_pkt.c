/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gparser_pkt.c
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
#include <gparser_pkt.h>
#include <glog.h>
#include <gbit.h>
/******************************************************************************
 * Declaration                                                                *
 *****************************************************************************/
#define LEN_VLAN_LLC_OTHER (sizeof(VlanTagHdr) + sizeof(EthLlc) + sizeof(EthLlcOther))

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
PacketCount pc;
int    gre_parse = 0;    /* 1: parse gre; 0: no parse gre */





/******************************************************************************
 * Local  function define                                                     *
 *****************************************************************************/
extern int  DecodeIP(const uint8_t * pkt, const uint32_t len, Packet * p);
extern int  DecodeVlan(const uint8_t * pkt, const uint32_t len, Packet * p);

/*
 * Function: DecodeGRE(uint8_t *, uint32_t, Packet *)
 *
 * Purpose: Decode Generic Routing Encapsulation Protocol
 *          This will decode normal GRE and PPTP GRE.
 *
 * Arguments: pkt => ptr to the packet data
 *            len => length from here to the end of the packet
 *            p   => pointer to decoded packet struct
 *
 * Returns: void function
 *
 * Notes: see RFCs 1701, 2784 and 2637
 */

PacketCount *get_gre_parse()
{
    return &pc;
}

static int DecodeGRE(const unsigned char *pkt, const uint32_t len, Packet *p)
{
    uint32_t hlen;    /* GRE header length */
    uint32_t payload_len;

    if (len < GRE_HEADER_LEN)
    {

        return 1;
    }

    if (p->encapsulated)
    {

        return 1;
    }

    /* Note: Since GRE doesn't have a field to indicate header length and
     * can contain a few options, we need to walk through the header to
     * figure out the length
     */

    p->greh = (GREHdr *)pkt;
    hlen = GRE_HEADER_LEN;

    switch (GRE_VERSION(p->greh))
    {
        case 0x00:
            /* these must not be set */
            if (GRE_RECUR(p->greh) || GRE_FLAGS(p->greh))
            {

                return 1;
            }

            if (GRE_CHKSUM(p->greh) || GRE_ROUTE(p->greh))
                hlen += GRE_CHKSUM_LEN + GRE_OFFSET_LEN;

            if (GRE_KEY(p->greh))
                hlen += GRE_KEY_LEN;

            if (GRE_SEQ(p->greh))
                hlen += GRE_SEQ_LEN;

            /* if this flag is set, we need to walk through all of the
             * Source Route Entries */
            if (GRE_ROUTE(p->greh))
            {
                uint16_t sre_addrfamily;
                uint8_t sre_offset;
                uint8_t sre_length;
                const uint8_t *sre_ptr;

                sre_ptr = (unsigned char*)pkt + hlen;

                while (1)
                {
                    hlen += GRE_SRE_HEADER_LEN;
                    if (hlen > len)
                        break;

                    sre_addrfamily = ntohs(*((uint16_t *)sre_ptr));
                    sre_ptr += sizeof(sre_addrfamily);

                    sre_offset = *((uint8_t *)sre_ptr);
                    sre_ptr += sizeof(sre_offset);

                    sre_length = *((uint8_t *)sre_ptr);
                    sre_ptr += sizeof(sre_length);

                    if ((sre_addrfamily == 0) && (sre_length == 0))
                        break;

                    hlen += sre_length;
                    sre_ptr += sre_length;
                }
            }

            break;

        /* PPTP */
        case 0x01:
            /* these flags should never be present */
            if (GRE_CHKSUM(p->greh) || GRE_ROUTE(p->greh) || GRE_SSR(p->greh)
||GRE_RECUR(p->greh) || GRE_V1_FLAGS(p->greh))
            {

                return 1;
            }

            /* protocol must be 0x880B - PPP */
            if (GRE_PROTO(p->greh) != GRE_TYPE_PPP)
            {

                return 1;
            }

            /* this flag should always be present */
            if (!(GRE_KEY(p->greh)))
            {

                return 1;
            }

            hlen += GRE_KEY_LEN;

            if (GRE_SEQ(p->greh))
                hlen += GRE_SEQ_LEN;

            if (GRE_V1_ACK(p->greh))
                hlen += GRE_V1_ACK_LEN;

            break;

        default:

            return 1;
    }

    if (hlen > len)
    {

        return 1;
    }

   /* PushLayer(PROTO_GRE, p, pkt, hlen);*/
    payload_len = len - hlen;

    /* Send to next protocol decoder */
    /* As described in RFC 2784 the possible protocols are listed in
     * RFC 1700 under "ETHER TYPES"
     * See also "Current List of Protocol Types" in RFC 1701
     */
    switch (GRE_PROTO(p->greh))
    {
        case ETHERNET_TYPE_IP:
            return DecodeIP(pkt + hlen, payload_len, p);
        /* not sure if this occurs, but 802.1q is an Ether type */
        case ETHERNET_TYPE_8021Q:
            return DecodeVlan(pkt + hlen, payload_len, p);
        default:
            // TBD add decoder drop event for unknown gre/eth type
            pc.other++;
            p->data = pkt + hlen;
            p->dsize = (uint16_t)payload_len;
            return 1;
    }
}


int  DecodeVlan(const uint8_t * pkt, const uint32_t len, Packet * p)
{
    pc.vlan++;

    if( !gre_parse)
    {
    if (p->greh != NULL)
        pc.gre_vlan++;
    }


    if(len < sizeof(VlanTagHdr))
    {

        // TBD add decoder drop event for VLAN hdr len issue
        pc.discards++;
        p->iph = NULL;
        return 1;
    }

    p->vh = (VlanTagHdr *) pkt;

    /* check to see if we've got an encapsulated LLC layer
     * http://www.geocities.com/billalexander/ethernet.html
     */
    if(ntohs(p->vh->vth_proto) <= ETHERNET_MAX_LEN_ENCAP)
    {
        if(len < sizeof(VlanTagHdr) + sizeof(EthLlc))
        {
            pc.discards++;
            p->iph = NULL;
            return 1;
        }

        p->ehllc = (EthLlc *) (pkt + sizeof(VlanTagHdr));

        if(p->ehllc->dsap == ETH_DSAP_IP && p->ehllc->ssap == ETH_SSAP_IP)
        {
            if ( len < LEN_VLAN_LLC_OTHER )
            {

                pc.discards++;
                p->iph = NULL;
                return 1;
            }

            p->ehllcother = (EthLlcOther *) (pkt + sizeof(VlanTagHdr) + sizeof(EthLlc));

          /*  PushLayer(PROTO_VLAN, p, pkt, sizeof(*p->vh));*/

            switch(ntohs(p->ehllcother->proto_id))
            {
                case ETHERNET_TYPE_IP:
                    return DecodeIP(p->pkt + LEN_VLAN_LLC_OTHER,
                             len - LEN_VLAN_LLC_OTHER, p);

                case ETHERNET_TYPE_8021Q:
                    pc.nested_vlan++;
                    return DecodeVlan(p->pkt + LEN_VLAN_LLC_OTHER,
                               len - LEN_VLAN_LLC_OTHER, p);
                default:
                    // TBD add decoder drop event for unknown vlan/eth type
                    pc.other++;
                    return 1;
            }
        }
    }
    else
    {
      //  PushLayer(PROTO_VLAN, p, pkt, sizeof(*p->vh));

        switch(ntohs(p->vh->vth_proto))
        {
            case ETHERNET_TYPE_IP:
                return DecodeIP(pkt + sizeof(VlanTagHdr),
                         len - sizeof(VlanTagHdr), p);
            case ETHERNET_TYPE_8021Q:
                pc.nested_vlan++;
                return DecodeVlan(pkt + sizeof(VlanTagHdr),
                           len - sizeof(VlanTagHdr), p);
            default:
                // TBD add decoder drop event for unknown vlan/eth type
                pc.other++;
                return 1;
        }
    }

    // TBD add decoder drop event for unknown vlan/llc type
    pc.other++;
    return 1;
}


/*
 * Function: DecodeTCP(uint8_t *, const uint32_t, Packet *)
 *
 * Purpose: Decode the TCP transport layer
 *
 * Arguments: pkt => ptr to the packet data
 *            len => length from here to the end of the packet
 *            p   => Pointer to packet decode struct
 *
 * Returns: void function
 */
static int  DecodeTCP(const uint8_t * pkt, const uint32_t len, Packet * p)
{

    uint32_t hlen;            /* TCP header length */

    if(len < TCP_HEADER_LEN)
    {
        p->tcph = NULL;
        pc.discards++;
        pc.tdisc++;

        return 1;
    }

    /* lay TCP on top of the data cause there is enough of it! */
    p->tcph = (TCPHdr *) pkt;

    /* multiply the payload offset value by 4 */
    hlen = TCP_OFFSET(p->tcph) << 2;

    if((hlen < TCP_HEADER_LEN)||(hlen > len))
    {
        p->tcph = NULL;
        pc.discards++;
        pc.tdisc++;

        return 1;
    }

    if(TCP_ISFLAGSET(p->tcph, (TH_SYN)))
    {
         p->tcph = NULL;
         pc.discards++;
         pc.tdisc++;
         return 1 ; /* 不处理SYN */
    }

    /* stuff more data into the printout data struct */
    p->sp = ntohs(p->tcph->th_sport);
    p->dp = ntohs(p->tcph->th_dport);


  /* PushLayer(PROTO_TCP, p, pkt, hlen); */

    p->tcp_options_len = (uint16_t)(hlen - TCP_HEADER_LEN);
    p->tcp_options_data = (p->tcp_options_len > 0)? pkt + TCP_HEADER_LEN:0;
    p->data = (uint8_t *) (pkt + hlen);    /* set the data pointer and size */
    p->dsize = (u_short)(len - hlen);
    p->proto_bits |= PROTO_BIT__TCP;


    return 0;

}


static int DecodeUDP(const uint8_t * pkt, const uint32_t len, Packet * p)
{


    uint16_t uhlen;
    //uint8_t  fragmented_udp_flag;

    if (p->proto_bits & (PROTO_BIT__TEREDO | PROTO_BIT__GTP))
        p->outer_udph = p->udph;

    if(len < sizeof(UDPHdr))
    {

        return 1;
    }

    /* set the ptr to the start of the UDP header */
    p->inner_udph = p->udph = (UDPHdr *) pkt;

    if (!p->frag_flag)
    {
        uhlen = ntohs(p->udph->uh_len);
    }
    else
    {

        uint16_t ip_len = ntohs(GET_IPH_LEN(p));
            /* Don't forget, IP_HLEN is a word - multiply x 4 */
        uhlen = ip_len - (GET_IPH_HLEN(p) * 4 );
        //fragmented_udp_flag = 1;
    }

    /* verify that the header len is a valid value */
    if(uhlen < UDP_HEADER_LEN)
    {

        return 1;
    }

    /* make sure there are enough bytes as designated by length field */
    if(uhlen > len)
    {

        return 1;
    }
    else if(uhlen < len)
    {

        return 1;
    }



    /* fill in the printout data structs */
    p->sp = ntohs(p->udph->uh_sport);
    p->dp = ntohs(p->udph->uh_dport);


    p->data = (uint8_t *) (pkt + UDP_HEADER_LEN);

    /* length was validated up above */
    p->dsize = uhlen - UDP_HEADER_LEN;

    p->proto_bits |= PROTO_BIT__UDP;

    return 0;
}

//--------------------------------------------------------------------
// decode.c::IP4 decoder
//--------------------------------------------------------------------

/*
 * Function: DecodeIP(uint8_t *, const uint32_t, Packet *)
 *
 * Purpose: Decode the IP network layer
 *
 * Arguments: pkt => ptr to the packet data
 *            len => length from here to the end of the packet
 *            p   => pointer to the packet decode struct
 *
 * Returns: void function
 */
int  DecodeIP(const uint8_t * pkt, const uint32_t len, Packet * p)
{
    uint32_t ip_len; /* length from the start of the ip hdr to the pkt end */
    uint32_t hlen;   /* ip header length */

    pc.ip++;

    if( !gre_parse)
    {
    if (p->greh != NULL)
        pc.gre_ip++;
    }

     /* do a little validation */

    if(len < IP_HEADER_LEN)
    {
        p->iph = NULL;
        pc.discards++;
        pc.ipdisc++;
        return 1;
    }


    p->iph = (IPHdr *)pkt;

    if(IP_VER(p->iph) != 4)
    {
        p->iph = NULL;
        pc.discards++;
        pc.ipdisc++;
        return 2;
    }

    /* get the IP datagram length */
    ip_len = ntohs(p->iph->ip_len);

    /* get the IP header length */
    hlen = IP_HLEN(p->iph) << 2;

    /* header length sanity check */

    if((hlen < IP_HEADER_LEN)||(ip_len > len)||(ip_len < hlen))
    {
//        printf("\n\n\nhlen  : %d\n", hlen);
//        printf("ip_len    : %d\n", ip_len);
//        printf("len       : %d\n\n\n\n", len);


        p->iph = NULL;
        pc.discards++;
        pc.ipdisc++;
        return 3;

    }


    /* test for IP options */
    p->ip_options_len = (uint16_t)(hlen - IP_HEADER_LEN);

    /* set the real IP length for logging */
    p->actual_ip_len = (uint16_t) ip_len;

    /* set the remaining packet length */
    ip_len -= hlen;

    /* check for fragmented packets */
    p->frag_offset = ntohs(p->iph->ip_off);

    /*
     * get the values of the reserved, more
     * fragments and don't fragment flags
     */
    p->rf = (uint8_t)((p->frag_offset & 0x8000) >> 15);
    p->df = (uint8_t)((p->frag_offset & 0x4000) >> 14);
    p->mf = (uint8_t)((p->frag_offset & 0x2000) >> 13);

    /* mask off the high bits in the fragment offset field */
    p->frag_offset &= 0x1FFF;


    if(p->frag_offset || p->mf)
    {
        /* set the packet fragment flag */
        p->frag_flag = 1;
        p->ip_frag_start = pkt + hlen;
        p->ip_frag_len = (uint16_t)ip_len;
        pc.frags++;
    }
    else
    {
        p->frag_flag = 0;
    }

    /* Set some convienience pointers */
    p->ip_data = pkt + hlen;
    p->ip_dsize = (u_short) ip_len;

    /* if this packet isn't a fragment
     * or if it is, its a UDP packet and offset is 0 */
     /* only support tcp/udp/gre */
    if(!(p->frag_flag) ||
            (p->frag_flag && (p->frag_offset == 0) &&
            (p->iph->ip_proto == IPPROTO_UDP)))
    {

        switch(p->iph->ip_proto)
        {
            case IPPROTO_TCP:
                pc.tcp++;
                return DecodeTCP(pkt + hlen, ip_len, p);

            case IPPROTO_UDP:
                pc.udp++;
               return DecodeUDP(pkt + hlen, ip_len, p);

            case IPPROTO_GRE:
                if( !gre_parse)
               {
                pc.gre++;
                return DecodeGRE(pkt + hlen, ip_len, p);
                }

            default:
                pc.other++;
                pc.discards++;
                pc.ipdisc++;
                p->data = pkt + hlen;
                p->dsize = (u_short) ip_len;
                return 4;
        }
    }
    else
    {
        /* set the payload pointer and payload size */
        p->data = pkt + hlen;
        p->dsize = (u_short) ip_len;
        return 5;
    }
}

static int DecodeGTP_U(const uint8_t *pkt, uint32_t len, Packet *p)
{
    uint32_t header_len;
    uint8_t  next_hdr_type;
    uint8_t  version;
    uint8_t  ip_ver;
    GTPHdr *hdr;
    hdr = (GTPHdr *) pkt;
    if (len < GTP_MIN_LEN)
       return 1;
    if ( hdr->type != 255)
       return 2;
    if (!(hdr->flag & 0x10))
       return 3;
    version = (hdr->flag & 0xE0) >> 5;
    switch (version)
    {
    case 0: /*GTP v0*/
        header_len = GTP_V0_HEADER_LEN;
        if (len < header_len)
        {
            return 4;
        }
        p->proto_bits |= PROTO_BIT__GTP;
        if (len != ((unsigned int)ntohs(hdr->length) + header_len))
        {
            return 4;
        }
        break;
    case 1: /*GTP v1*/
        if (hdr->flag & 0x07)
        {
            header_len = GTP_V1_HEADER_LEN;
            if (len < header_len)
            {
                return 4;
            }
            next_hdr_type = *(pkt + header_len - 1);
            while (next_hdr_type)
            {
                uint16_t ext_hdr_len;
                if (len < header_len + 4)
                {
                    return 5;
                }
                ext_hdr_len = *(pkt + header_len);
                if (!ext_hdr_len)
                {
                    return 6;
                }
                header_len += ext_hdr_len * 4;
                if (len < header_len)
                {
                    return 7;
                }
                next_hdr_type = *(pkt + header_len - 1);
            }
        }
        else
            header_len = GTP_MIN_LEN;
        p->proto_bits |= PROTO_BIT__GTP;
        #if 0
        if (len != ((unsigned int)ntohs(hdr->length) + GTP_MIN_LEN))
        {
            return 8;
        }
        #endif
        break;
    default:
        return 9;
    }
    len -=  header_len;
    if (len > 0)
    {
        ip_ver = *(pkt+header_len) & 0xF0;
        return DecodeIP(pkt+header_len, len, p);
    }
    return -1;
}

static int DecodeMPLS(const uint8_t* pkt, const uint32_t len, Packet* p)
{
    uint32_t* tmpMplsHdr;
    uint32_t mpls_h;
    uint32_t label;
    uint32_t mlen = 0;

    uint8_t exp;
    uint8_t bos = 0;
    uint8_t ttl;


    pc.mpls++;
    tmpMplsHdr = (uint32_t *) pkt;
    p->mpls = NULL;

    int cnt = 0;
    while ((!bos) && cnt < 3)     //the most parse mpls layer is three
    {
        cnt ++;
        if(len < MPLS_HEADER_LEN)
        {
            pc.discards++;
            p->iph = NULL;
            return -1;
        }

        mpls_h  = ntohl(*tmpMplsHdr);
        ttl = (uint8_t)(mpls_h & 0x000000FF);
        mpls_h = mpls_h>>8;
        bos = (uint8_t)(mpls_h & 0x00000001);
        exp = (uint8_t)(mpls_h & 0x0000000E);
        label = (mpls_h>>4) & 0x000FFFFF;

        if((label<NUM_RESERVED_LABELS))
            return -2;

        if( bos )
        {
            p->mplsHdr.label = label;
            p->mplsHdr.exp = exp;
            p->mplsHdr.bos = bos;
            p->mplsHdr.ttl = ttl;
            /**
            p->mpls = &(p->mplsHdr);
            **/
            p->mpls = tmpMplsHdr;
        }
        tmpMplsHdr++;
    }   /* while bos not 1, peel off more labels */

    mlen = (uint8_t*)tmpMplsHdr - pkt;
    mlen = len - mlen;

    IPHdr *iphdr =(IPHdr *) tmpMplsHdr;
    int ret = 0;
    switch (IP_VER(iphdr))
    {
        case MPLS_TYPE_IPV4:
            ret = DecodeIP((uint8_t *)tmpMplsHdr, mlen, p);
            break;
        default:
            return -6;
            break;
    }
    return ret;
}


/******************************************************************************
 * Global function define                                                     *
 *****************************************************************************/

/*
 * Function: gparser_pkt_parse(gpacket_info_t *pkt_info, void *pkt_buf, int pkt_len)
 *
 * Purpose: Decode packets, in level 2,3,4
 *
 * Arguments: pkt_info => pointer to the decoded packet struct
 *                   pkt_buf =>  pointer to the real live packet data
                      pkt_type=> packet type: 1:ethernet,2:pos,3:hdlc  暂定，待讨论
                      PktHdr_t => packet sum info
 *
 * Returns: parser ret.
        0: parse suc;
        1: erro packet ;
        2: 2level no pase;
        3: 3level no pase;
        4: 4level no pase;
        5: ip devid
 */

static int _gparser_pkt_parse(int pkt_type,PktHdr_t *pkthdr ,void *pkt_buf, Packet *p )
{
    int cap_len = pkthdr->caplen;
    int ret = 0;


    /*判断链路类型。支持POS/Ethernet */
    switch( pkt_type )
    {
        case PKT_LINK_ETHERNET:
            goto  eth_parse;
            break;
        case PKT_LINK_POS:
            goto  pos_parse;
            break;
        case PKT_LINK_HDLC:
            goto  hdlc_parse;
            break;
        default:
             pc.discards++;
            return 2;
     }


    /* ethernet  head parse*/
 eth_parse:
    pc.eth++;
    pc.total_processed++;

    p->pkth = pkthdr;
    p->pkt   = (uint8_t*)pkt_buf;

      /* do a little validation */
    #if 0
    if(cap_len < ETHERNET_HEADER_LEN)
    {

        pc.discards++;
        pc.ethdisc++;

        return 1;
    }
    #endif
    p->eh = (EtherHdr *) pkt_buf;
    /* grab out the network type ,only support ip,802.1q*/
    switch(ntohs(p->eh->ether_type))
    {
        case ETHERNET_TYPE_IP:
            ret =  DecodeIP(p->pkt + ETHERNET_HEADER_LEN,
                    cap_len - ETHERNET_HEADER_LEN, p);

            GLOG_PRINT(GLOG_DEBUG,1,"ip packet parse:\n");
            goto Protocol;
        case ETHERNET_TYPE_8021Q:
            ret =  DecodeVlan(p->pkt + ETHERNET_HEADER_LEN,
                    cap_len - ETHERNET_HEADER_LEN, p);
            GLOG_PRINT(GLOG_DEBUG,1,"vlan packet parse:\n");
            goto Protocol;
        case ETHERNET_TYPE_MPLS_UNICAST:
            GLOG_PRINT(GLOG_DEBUG,1,"mpls packet parse:\n");
            ret =  DecodeMPLS(p->pkt + ETHERNET_HEADER_LEN,cap_len - ETHERNET_HEADER_LEN, p);
            goto Protocol;
        default:
            // TBD add decoder drop event for unknown eth type
            pc.other++;
            return 3;
    }


    /* POS  head parse*/
pos_parse:
hdlc_parse:
    pc.eth++;
    pc.total_processed++;

    p->pkth = pkthdr;
    p->pkt   = (uint8_t*)pkt_buf;

      /* do a little validation */
    if(cap_len < POS_LEN)
    {
        pc.discards++;
        pc.ethdisc++;
        return 1;
    }
    p->posh = (PosHdr *) pkt_buf;

    /* grab out the network type ,only support ip,802.1q*/
    switch(ntohs(p->posh->ether_type))
    {
        case PPP_TYPE_IP:
            ret =   DecodeIP(p->pkt + 4,
                    cap_len - 4, p);
            GLOG_PRINT(GLOG_DEBUG,1,"pos packet parse:\n");
            goto Protocol;
        case ETHERNET_TYPE_8021Q:
             ret =  DecodeVlan(p->pkt + ETHERNET_HEADER_LEN,
                    cap_len - ETHERNET_HEADER_LEN, p);
             GLOG_PRINT(GLOG_DEBUG,1,"vlan packet parse:\n");
             goto Protocol;
        default:
            // TBD add decoder drop event for unknown eth type
            pc.other++;
            return 3;
    }
     return 1;


Protocol:
    if( ret !=0)
    {
        return ret;
    }

    pc.total++;
    if(p->iph->ip_off & 0x3FFF){
        pc.frament++;
    }
    //http:
    if(p->iph->ip_proto == IPPROTO_TCP)
    {
        #if 0
        printf("TCP ret = %d src ip : %s,src port : %d\n",ret,inet_ntoa((p->iph->ip_src)),p->tcph->th_sport);
        printf("TCP ret = %d des ip : %s,des port : %d\n",ret,inet_ntoa((p->iph->ip_dst)),p->tcph->th_dport);
        printf("\n\n\n");
        #endif
        if(ntohs(p->tcph->th_dport) == 80){
            p->usSrvID = SRV_HTTP;
        }
    }
    else if(p->iph->ip_proto == IPPROTO_UDP)
    {
        #if 0
        printf("UDP ret = %d src ip : %s,src port : %d\n",ret,inet_ntoa((p->iph->ip_src)),p->udph->uh_sport);
        printf("UDP ret = %d des ip : %s,des port : %d\n",ret,inet_ntoa((p->iph->ip_dst)),p->udph->uh_dport);
        printf("\n\n\n");
        #endif
        if(   (ntohs(p->udph->uh_dport) == 1812)||(ntohs(p->udph->uh_dport) == 1813)
           || (ntohs(p->udph->uh_sport) == 1812)||(ntohs(p->udph->uh_sport) == 1813))
        {
            p->usSrvID = SRV_RADIUS;
        }else if( (ntohs(p->udph->uh_dport) == 2152) || (ntohs(p->udph->uh_sport) == 2152))
        {
            p->usSrvID = SRV_GTP_U;
        }else if( (ntohs(p->udph->uh_dport) == 2123) || (ntohs(p->udph->uh_sport) == 2123))
        {
            p->usSrvID = SRV_GTP_C;
        }
    }
    else
    {
        p->usSrvID = SRV_UNKNOW;
    }
    return ret;
}
int gparse_gtpu_parse(Packet *p)
{
    return DecodeGTP_U(p->data, p->udph->uh_len, p);
}

int gparse_pkt_parse(PktHdr_t *pkt_hdr, gpacket_info_t *pkt_info, int pkt_type, void *pkt_buf, int pkt_len)
{
    pkt_hdr->ts     = 0;
    pkt_hdr->us     = 0;
    pkt_hdr->caplen = pkt_len;
    pkt_hdr->pktlen = pkt_len;
    pkt_hdr->device_index = 0;
    pkt_hdr->flags  = 0;
    return _gparser_pkt_parse(pkt_type, pkt_hdr, pkt_buf, pkt_info);
}

