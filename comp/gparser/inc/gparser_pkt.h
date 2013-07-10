/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      gparser_pkt.h
 *
 * Brief:
 *
 *
 * History:
 *
 *****************************************************************************/

#ifndef _GPARSER_PKT_INC_
#define _GPARSER_PKT_INC_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Macro & Enum define                                                        *
 *****************************************************************************/
/* link type*/
 #define PKT_LINK_ETHERNET      1
 #define PKT_LINK_POS           2
 #define PKT_LINK_HDLC          3

#define POS_LEN 4

#define ETHERNET_HEADER_LEN             14
#define ETHERNET_MAX_LEN_ENCAP       1518    /* 802.3 (+LLC) or ether II */

#define ETHERNET_MTU                  1500
#define ETHERNET_TYPE_IP              0x0800
#define ETHERNET_TYPE_ARP             0x0806
#define ETHERNET_TYPE_REVARP          0x8035
#define ETHERNET_TYPE_EAPOL           0x888e
#define ETHERNET_TYPE_IPV6            0x86dd
#define ETHERNET_TYPE_IPX             0x8137
#define ETHERNET_TYPE_PPPoE_DISC      0x8863 /* discovery stage */
#define ETHERNET_TYPE_PPPoE_SESS      0x8864 /* session stage */
#define ETHERNET_TYPE_8021Q           0x8100
#define ETHERNET_TYPE_LOOP            0x9000
#define ETHERNET_TYPE_MPLS_UNICAST    0x8847
#define ETHERNET_TYPE_MPLS_MULTICAST  0x8848

#define PPP_TYPE_IP                   0X0021

/* otherwise defined in /usr/include/ppp_defs.h */
#define MPLS_TYPE_IPV4          4
#define MPLS_HEADER_LEN         4
#define NUM_RESERVED_LABELS    16


#define IP_HEADER_LEN           20
#define TCP_HEADER_LEN          20
#define UDP_HEADER_LEN          8
#define ICMP_HEADER_LEN         4
#define ICMP_NORMAL_LEN         8

#define IP_VER(iph)    (((iph)->ip_verhl & 0xf0) >> 4)
#define IP_HLEN(iph)   ((iph)->ip_verhl & 0x0f)

/* more macros for TCP offset */
#define TCP_OFFSET(tcph)        (((tcph)->th_offx2 & 0xf0) >> 4)
#define TCP_X2(tcph)            ((tcph)->th_offx2 & 0x0f)

#define TCP_ISFLAGSET(tcph, flags) (((tcph)->th_flags & (flags)) == (flags))


#define PROTO_BIT__NONE     0x0000
#define PROTO_BIT__IP       0x0001
#define PROTO_BIT__ARP      0x0002
#define PROTO_BIT__TCP      0x0004
#define PROTO_BIT__UDP      0x0008
#define PROTO_BIT__ICMP     0x0010
#define PROTO_BIT__TEREDO   0x0020
#define PROTO_BIT__GTP      0x0040
#define PROTO_BIT__ALL      0xffff

#define GET_IPH_PROTO(p) (p)->iph->ip_proto
#define GET_IPH_TOS(p) (p)->iph->ip_tos
#define GET_IPH_LEN(p) (p)->iph->ip_len
#define GET_IPH_TTL(p) (p)->iph->ip_ttl
#define GET_IPH_VER(p) (((p)->iph->ip_verhl & 0xf0) >> 4)
#define GET_IPH_ID(p) (p)->iph->ip_id
#define GET_IPH_OFF(p) (p)->iph->ip_off
#define GET_IPH_HLEN(p) ((p)->iph->ip_verhl & 0x0f)
/* GRE*/

#define GRE_TYPE_TRANS_BRIDGING 0x6558
#define GRE_TYPE_PPP            0x880B

#define GRE_HEADER_LEN 4
#define GRE_CHKSUM_LEN 2
#define GRE_OFFSET_LEN 2
#define GRE_KEY_LEN 4
#define GRE_SEQ_LEN 4
#define GRE_SRE_HEADER_LEN 4

#define GRE_CHKSUM(x)  (x->flags & 0x80)
#define GRE_ROUTE(x)   (x->flags & 0x40)
#define GRE_KEY(x)     (x->flags & 0x20)
#define GRE_SEQ(x)     (x->flags & 0x10)
#define GRE_SSR(x)     (x->flags & 0x08)
#define GRE_RECUR(x)   (x->flags & 0x07)
#define GRE_VERSION(x)   (x->version & 0x07)
#define GRE_FLAGS(x)     (x->version & 0xF8)
#define GRE_PROTO(x)  ntohs(x->ether_type)

/* GRE version 1 used with PPTP */
#define GRE_V1_HEADER_LEN 8
#define GRE_V1_ACK_LEN 4
#define GRE_V1_FLAGS(x)  (x->version & 0x78)
#define GRE_V1_ACK(x)    (x->version & 0x80)

/* VLAN*/
#define ETH_DSAP_SNA                  0x08    /* SNA */
#define ETH_SSAP_SNA                  0x00    /* SNA */
#define ETH_DSAP_STP                  0x42    /* Spanning Tree Protocol */
#define ETH_SSAP_STP                  0x42    /* Spanning Tree Protocol */
#define ETH_DSAP_IP                   0xaa    /* IP */
#define ETH_SSAP_IP                   0xaa    /* IP */

// these are bits in th_flags,TCP flags:
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80
#define TH_RES2 TH_ECE  // TBD TH_RES* should be deleted (see log.c)
#define TH_RES1 TH_CWR
#define TH_NORESERVED (TH_FIN|TH_SYN|TH_RST|TH_PUSH|TH_ACK|TH_URG)

#define SRV_UNKNOW  0
#define SRV_HTTP    1
#define SRV_RADIUS  2
#define SRV_GTP_C   3
#define SRV_GTP_U   4
#define GTP_MIN_LEN 8
#define GTP_V0_HEADER_LEN 20
#define GTP_V1_HEADER_LEN 12
/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/

/* struct to collect packet statistics */
typedef struct _PacketCount
{
    uint64_t total_from_daq;
    uint64_t total_processed;

    uint64_t s5tcp1;
    uint64_t s5tcp2;
    uint64_t ipv6opts;
    uint64_t eth;
    uint64_t ethdisc;
    uint64_t ipv6disc;
    uint64_t ip6ext;
    uint64_t other;
    uint64_t tcp;
    uint64_t udp;
    uint64_t icmp;
    uint64_t arp;
#ifndef NO_NON_ETHER_DECODER
    uint64_t eapol;
#endif
    uint64_t vlan;
    uint64_t nested_vlan;
    uint64_t ipv6;
    uint64_t ipv6_up;
    uint64_t ipv6_upfail;
    uint64_t frag6;
    uint64_t icmp6;
    uint64_t tdisc;
    uint64_t udisc;
    uint64_t tcp6;
    uint64_t udp6;
    uint64_t teredo;
    uint64_t ipdisc;
    uint64_t icmpdisc;
    uint64_t embdip;
    uint64_t ip;
    uint64_t ipx;
    uint64_t ethloopback;

    uint64_t invalid_checksums;
    uint64_t bad_ttl;

    uint64_t ip4ip4;
    uint64_t ip4ip6;
    uint64_t ip6ip4;
    uint64_t ip6ip6;

    uint64_t gre;
    uint64_t gre_ip;
    uint64_t gre_eth;
    uint64_t gre_arp;
    uint64_t gre_ipv6;
    uint64_t gre_ipv6ext;
    uint64_t gre_ipx;
    uint64_t gre_loopback;
    uint64_t gre_vlan;
    uint64_t gre_ppp;


    uint64_t discards;
    uint64_t alert_pkts;
    uint64_t log_pkts;
    uint64_t pass_pkts;

    uint64_t match_limit;
    uint64_t queue_limit;
    uint64_t log_limit;
    uint64_t event_limit;
    uint64_t alert_limit;

    uint64_t frags;           /* number of frags that have come in */
    uint64_t frag_trackers;   /* number of tracking structures generated */
    uint64_t rebuilt_frags;   /* number of packets rebuilt */
    uint64_t frag_incomp;     /* number of frags cleared due to memory issues
*/
    uint64_t frag_timeout;    /* number of frags cleared due to timeout */
    uint64_t rebuild_element; /* frags that were element of rebuilt pkt */
    uint64_t frag_mem_faults; /* number of times the memory cap was hit */

    uint64_t tcp_stream_pkts; /* number of packets tcp reassembly touches */
    uint64_t rebuilt_tcp;     /* number of phoney tcp packets generated */
    uint64_t tcp_streams;     /* number of tcp streams created */
    uint64_t rebuilt_segs;    /* number of tcp segments used in rebuilt pkts */
    uint64_t queued_segs;     /* number of tcp segments stored for rebuilt
pkts */
    uint64_t str_mem_faults;  /* number of times the stream memory cap was
hit */

#ifdef TARGET_BASED
    uint64_t attribute_table_reloads; /* number of times attribute table was
reloaded. */
#endif

#ifndef NO_NON_ETHER_DECODER
#ifdef DLT_IEEE802_11
  /* wireless statistics */
    uint64_t wifi_mgmt;
    uint64_t wifi_data;
    uint64_t wifi_control;
    uint64_t assoc_req;
    uint64_t assoc_resp;
    uint64_t reassoc_req;
    uint64_t reassoc_resp;
    uint64_t probe_req;
    uint64_t probe_resp;
    uint64_t beacon;
    uint64_t atim;
    uint64_t dissassoc;
    uint64_t auth;
    uint64_t deauth;
    uint64_t ps_poll;
    uint64_t rts;
    uint64_t cts;
    uint64_t ack;
    uint64_t cf_end;
    uint64_t cf_end_cf_ack;
    uint64_t data;
    uint64_t data_cf_ack;
    uint64_t data_cf_poll;
    uint64_t data_cf_ack_cf_poll;
    uint64_t cf_ack;
    uint64_t cf_poll;
    uint64_t cf_ack_cf_poll;
#endif
#endif  // NO_NON_ETHER_DECODER

    uint64_t mpls;
    uint64_t mplsdisc;
    uint64_t total;
    uint64_t frament;
} PacketCount;

/* The DAQ packet header structure passed to DAQ Analysis Functions.  This should NEVER be modified by user applications. */
typedef struct _pkthdr
{
    uint32_t ts;      /* Timestamp */
    uint32_t us;
    uint32_t caplen;        /* Length of the portion present */
    uint32_t pktlen;        /* Length of this packet (off wire) */
    int device_index;       /* Index of the receiving interface. */
    uint32_t flags;         /* Flags for the packet (DAQ_PKT_FLAG_*) */
}  PktHdr_t;

/* GRE related stuff */
typedef struct _GREHdr
{
    uint8_t flags;
    uint8_t version;
    uint16_t ether_type;

} GREHdr;

typedef struct _VlanTagHdr
{
    uint16_t vth_pri_cfi_vlan;
    uint16_t vth_proto;  /* protocol field... */
} VlanTagHdr;

typedef struct _PosHdr
{
    uint8_t  pos_head[2];
    uint16_t ether_type;

} PosHdr;

typedef struct _EthLlc
{
    uint8_t dsap;
    uint8_t ssap;
} EthLlc;

typedef struct _EthLlcOther
{
    uint8_t ctrl;
    uint8_t org_code[3];
    uint16_t proto_id;
} EthLlcOther;

/*
 * Ethernet header
 */

typedef struct _EtherHdr
{
    uint8_t ether_dst[6];
    uint8_t ether_src[6];
    uint16_t ether_type;
} EtherHdr;

typedef struct _MplsHdr
{
    uint32_t label;
    uint8_t  exp;
    uint8_t  bos;
    uint8_t  ttl;
} MplsHdr;


/* IP V4  head */
typedef struct _IPHdr
{
    uint8_t ip_verhl;      /* version & header length */
    uint8_t ip_tos;        /* type of service */
    uint16_t ip_len;       /* datagram length */
    uint16_t ip_id;        /* identification  */
    uint16_t ip_off;       /* fragment offset */
    uint8_t ip_ttl;        /* time to live field */
    uint8_t ip_proto;      /* datagram protocol */
    uint16_t ip_csum;      /* checksum */
    struct in_addr ip_src;  /* source IP */
    struct in_addr ip_dst;  /* dest IP   */
} IPHdr;

/* TCP  head */
typedef struct _TCPHdr
{
    uint16_t th_sport;     /* source port */
    uint16_t th_dport;     /* destination port */
    uint32_t th_seq;       /* sequence number */
    uint32_t th_ack;       /* acknowledgement number */
    uint8_t th_offx2;      /* offset and reserved */
    uint8_t th_flags;
    uint16_t th_win;       /* window */
    uint16_t th_sum;       /* checksum */
    uint16_t th_urp;       /* urgent pointer */

}TCPHdr;

/* UDP  head */
typedef struct _UDPHdr
{
    uint16_t uh_sport;
    uint16_t uh_dport;
    uint16_t uh_len;
    uint16_t uh_chk;

}UDPHdr;

typedef struct _GTPHdr
{
    uint8_t  flag;              /* flag: version (bit 6-8), PT (5), E (3), S (2), PN (1) */
    uint8_t  type;              /* message type */
    uint16_t length;            /* length */

} GTPHdr;
/*
*   原始包解析后的内容填充
*/
typedef struct _Packet
{
    const PktHdr_t *pkth;    // packet meta data
    const uint8_t *pkt;         // raw packet data

    //vvv------------------------------------------------
    // TODO convenience stuff to be refactored for layers
    //^^^------------------------------------------------

    //vvv-----------------------------

    /* 2 level   一期只考虑ethernet和VLAN,其他报文暂不解析 */
   /* EtherARP *ah;  arp 不涉及*/
    const PosHdr *posh;
    const EtherHdr *eh;         /* standard TCP/IP/Ethernet/ARP headers */
    const VlanTagHdr *vh;
    EthLlc *ehllc;
    EthLlcOther *ehllcother;
    /*  const PPPoEHdr *pppoeh;       Encapsulated PPP of Ether header */
    const GREHdr *greh;

    MplsHdr   mplsHdr;
    uint32_t *mpls;     /*mpls报文*/

     /* 3 level */
    IPHdr *iph, *orig_iph;/* and orig. headers for ICMP_*_UNREACH family */
    const IPHdr *inner_iph;     /* if IP-in-IP, this will be the inner IP header */
    const IPHdr *outer_iph;     /* if IP-in-IP, this will be the outer IP header */
    TCPHdr *tcph, *orig_tcph;
    const UDPHdr *udph, *orig_udph;
    const UDPHdr *inner_udph;   /* if Teredo + UDP, this will be the inner UDP header */
    const UDPHdr *outer_udph;   /* if Teredo + UDP, this will be the outer UDP header */
    /* const ICMPHdr *icmph, *orig_icmph; */

    const uint8_t *data;        /* packet payload pointer */
    const uint8_t *ip_data;     /* IP payload pointer */
    const uint8_t *outer_ip_data;  /* Outer IP payload pointer */
    const uint8_t *ip_frag_start;
    const uint8_t *ip_options_data;
    const uint8_t *tcp_options_data;
    //^^^-----------------------------

    void *ssnptr;               /* for tcp session tracking info... */
    void *fragtracker;          /* for ip fragmentation tracking info... */
    void *flow;                 /* for flow info    */
    void *streamptr;            /* for tcp pkt dump */
    void *policyEngineData;

    /*/vvv-----------------------------
    IP4Hdr *ip4h, *orig_ip4h;
    IP6Hdr *ip6h, *orig_ip6h;
    ICMP6Hdr *icmp6h, *orig_icmp6h;

    IPH_API* iph_api;
    IPH_API* orig_iph_api;
    IPH_API* outer_iph_api;
    IPH_API* outer_orig_iph_api;

    IP4Hdr inner_ip4h, inner_orig_ip4h;
    IP6Hdr inner_ip6h, inner_orig_ip6h;
    IP4Hdr outer_ip4h, outer_orig_ip4h;
    IP6Hdr outer_ip6h, outer_orig_ip6h;

    MplsHdr   mplsHdr;
    */
    int family;
    int orig_family;
    int outer_family;
    int bytes_to_inspect;       /* Number of bytes to check against rules */
                                /* this is not set - always 0 (inspect all) */

    /* int ip_payload_len; */   /* Replacement for IP_LEN(p->iph->ip_len) << 2 */
    /* int ip_payload_off; */   /* IP_LEN(p->iph->ip_len) << 2 + p->data */
    //^^^-----------------------------

    uint32_t preprocessor_bits; /* flags for preprocessors to check */
    uint32_t preproc_reassembly_pkt_bits;

    uint32_t http_pipeline_count; /* Counter for HTTP pipelined requests */
    uint32_t packet_flags;      /* special flags for the packet */
    uint16_t proto_bits;

    //vvv-----------------------------
    uint16_t dsize;             /* packet payload size */
    uint16_t ip_dsize;          /* IP payload size */
    uint16_t alt_dsize;         /* the dsize of a packet before munging (used for log)*/
    uint16_t actual_ip_len;     /* for logging truncated pkts (usually by small snaplen)*/
    uint16_t outer_ip_dsize;    /* Outer IP payload size */
    //^^^-----------------------------

    uint16_t frag_offset;       /* fragment offset number */
    uint16_t ip_frag_len;
    uint16_t ip_options_len;
    uint16_t tcp_options_len;

    //vvv-----------------------------
    uint16_t sp;                /* source port (TCP/UDP) */
    uint16_t dp;                /* dest port (TCP/UDP) */
    uint16_t orig_sp;           /* source port (TCP/UDP) of original datagram */
    uint16_t orig_dp;           /* dest port (TCP/UDP) of original datagram */
    //^^^-----------------------------
    // and so on ...

  /*  int16_t application_protocol_ordinal; */

    uint8_t frag_flag;          /* flag to indicate a fragmented packet */
    uint8_t mf;                 /* more fragments flag */
    uint8_t df;                 /* don't fragment flag */
    uint8_t rf;                 /* IP reserved bit */

    uint8_t uri_count;          /* number of URIs in this packet */
    uint8_t error_flags;        /* flags indicate checksum errors, bad TTLs, etc. */
    uint8_t encapsulated;
    uint8_t GTPencapsulated;

    uint8_t ip_option_count;    /* number of options in this packet */
    uint8_t tcp_option_count;
    uint8_t ip6_extension_count;
    uint8_t ip6_frag_index;

    uint8_t ip_lastopt_bad;     /* flag to indicate that option decoding was
                                   halted due to a bad option */
    uint8_t tcp_lastopt_bad;    /* flag to indicate that option decoding was
                                   halted due to a bad option */

    uint8_t next_layer;         /* index into layers for next encap */

    uint32_t xtradata_mask;
    uint32_t per_packet_xtradata;

#if 0
#ifndef NO_NON_ETHER_DECODER
    const Fddi_hdr *fddihdr;    /* FDDI support headers */
    Fddi_llc_saps *fddisaps;
    Fddi_llc_sna *fddisna;
    Fddi_llc_iparp *fddiiparp;
    Fddi_llc_other *fddiother;

    const Trh_hdr *trh;         /* Token Ring support headers */
    Trh_llc *trhllc;
    Trh_mr *trhmr;

    Pflog1Hdr *pf1h;            /* OpenBSD pflog interface header - version 1 */
    Pflog2Hdr *pf2h;            /* OpenBSD pflog interface header - version 2 */
    Pflog3Hdr *pf3h;            /* OpenBSD pflog interface header - version 3 */


#ifdef DLT_LINUX_SLL
    const SLLHdr *sllh;         /* Linux cooked sockets header */
#endif
#ifdef DLT_IEEE802_11
    const WifiHdr *wifih;       /* wireless LAN header */
#endif
    const EtherEapol *eplh;     /* 802.1x EAPOL header */
    const EAPHdr *eaph;
    const uint8_t *eaptype;
    EapolKey *eapolk;
#endif



    // nothing after this point is zeroed ...
    Options ip_options[IP_OPTMAX];         /* ip options decode structure */
    Options tcp_options[TCP_OPTLENMAX];    /* tcp options decode struct */
    IP6Option ip6_extensions[IP6_EXTMAX];  /* IPv6 Extension References */

    const IP6RawHdr* raw_ip6h;  // innermost raw ip6 header
    Layer layers[LAYER_MAX];    /* decoded encapsulations */

    PseudoPacketType pseudo_type;    // valid only when PKT_PSEUDO is set
    uint16_t max_dsize;

    /**policyId provided in configuration file. Used for correlating configuration
     * with event output
     */
    uint16_t configPolicyId;
#endif

    uint16_t usSrvID;     /* protocol ID*/
    uint16_t usAppID;     /* aplication ID */
    uint16_t usGtpUD;     /* the flag of on and down*/
} Packet, gpacket_info_t;

/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/

/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int gparse_pkt_parse(PktHdr_t *pkt_hdr, gpacket_info_t *pkt_info, int pkt_type, void *pkt_buf, int pkt_len);
int gparse_gtpu_parse(gpacket_info_t *p);

static inline int    gparse_pkt_svrid(gpacket_info_t *packet_info)
{
    return packet_info->usSrvID;
}

static inline void * gparse_pkt_payload(gpacket_info_t *packet_info)
{
    return (void *) packet_info->data;
}

static inline uint16_t gparse_pkt_payload_size(gpacket_info_t *packet_info)
{
    return packet_info->dsize;
}

static inline int    gparse_pkt_ipver(gpacket_info_t *packet_info)
{
    if(packet_info->iph){
        return IP_VER(packet_info->iph);
    }else{
        return 0;
    }
}

static inline void * gparse_pkt_srcipaddr(gpacket_info_t *packet_info)
{
    if(packet_info->iph){
        return (void *) &packet_info->iph->ip_src;
    }else{
        return NULL;
    }
}

static inline void * gparse_pkt_dstipaddr(gpacket_info_t *packet_info)
{
    if(packet_info->iph){
        return (void *) &packet_info->iph->ip_dst;
    }else{
        return NULL;
    }
}
PacketCount *get_gre_parse();


#ifdef _cplusplus
}
#endif

#endif /* _GPARSER_PKT_INC_ */

/* End of gparser_pkt.h */

