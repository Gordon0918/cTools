/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      fw_intf.h
 *
 * Brief:
 *      framework interface header file
 *
 * History:
 *      24/10/2012  create dinglixing
 *****************************************************************************/

#ifndef _FW_INTF_INC_
#define _FW_INTF_INC_


#ifdef _cplusplus
extern "C" {
#endif

/******************************************************************************
 * Types & Defines                                                            *
 *****************************************************************************/
#define INTF_REQ_LINE (1024)
#define INTF_BUF_SIZE (1024*8)
#define INTF_ARG_NMAX (64)

enum {
    ERR_INTF_LOCAL  = 1,
    ERR_INTF_REMOTE = 2,
    ERR_INTF_TRANS  = 3,
};

typedef struct _fw_intf_sess_t {
    int   sock;
    char  req_line[INTF_REQ_LINE];
    int   req_argc;
    char *req_argv[INTF_ARG_NMAX];
    int   req_data_len;
    void *req_data;

    int   resp_result;
    int   resp_len;
    void *resp_buf;

    int   recv_len;
    void *recv_buf;
}fw_intf_sess_t;

typedef int (*fw_intf_req_handle_t)(fw_intf_sess_t *sess);
typedef int (*fw_intf_resp_handle_t)(int status, int resp_result, int ret_len, void *ret_dat, void *udata);

/******************************************************************************
 * Struct & Union define                                                      *
 *****************************************************************************/


/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int fw_intf_init(
    const char          *cmd_addr,
    const char          *cmd_port,
    fw_intf_req_handle_t cmd_proc);

int fw_intf_deinit(void);

int fw_intf_mainloop(void);
int fw_intf_exit(void);

int fw_intf_request(
    const char *addr,
    const char *port,
    int         argc, 
    char      **argv,
    int         dlen,
    void       *dbuf,
    fw_intf_resp_handle_t resp_proc, void *udata);



#ifdef _cplusplus
}
#endif

#endif /* _FW_INTF_INC_ */

/* End of fw_intf.h */

