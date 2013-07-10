/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *     fw_intf.c
 *
 * Brief:
 *
 *
 * History:
 *      24/10/2012 create dinglixing
 *****************************************************************************/

/******************************************************************************
 * Include files                                                              *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <fw_intf.h>

#include "public.h"


/******************************************************************************
 * Types & Defines                                                              *
 *****************************************************************************/

/******************************************************************************
 * Local veriables                                                            *
 *****************************************************************************/
static int                  intf_sock = -1;
static int                  intf_done = 0;
static void                *intf_recv_buff = NULL;
static void                *intf_resp_buff = NULL;
static fw_intf_sess_t       intf_sess;
static fw_intf_req_handle_t intf_req_proc = NULL;

/******************************************************************************
 * Local functions                                                            *
 *****************************************************************************/
static int intf_sock_listen(unsigned short port)
{
    int fd;
    int onoff;
    struct sockaddr_in inaddr;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
        perror("socket");
        return -1;
    }

    onoff = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &onoff,sizeof(onoff)) < 0) {
        perror("socket");
        close(fd);
        return -1;
    }

    memset((void *)&inaddr, 0, sizeof(struct sockaddr_in));
    inaddr.sin_family      = AF_INET;
    inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    inaddr.sin_port        = htons(port);

    if (bind(fd, (struct sockaddr*)&inaddr, sizeof(inaddr)) == -1) {
        close(fd);
        return -1;
    }

    if (listen(fd, 10) == -1) {
        close(fd);
        return -1;
    }

    return fd;
}

static int intf_sock_connect(const char *svr_ip, unsigned short svr_port)
{
    int                         sock;
    struct timeval              tv;
    static struct sockaddr_in   peer;

    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    peer.sin_port   = htons(svr_port);
    if( inet_pton(AF_INET, svr_ip, &peer.sin_addr) <= 0) {
        return -1;
    }

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if(connect(sock, (struct sockaddr*)&peer, sizeof(peer)) != 0) {
        close(sock);
        return -1;
    }

    tv.tv_sec  = 5;
    tv.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    return sock;
}

static int intf_sock_sendn(int sock, void *buf, int size)
{
    int  ret, lft;

    ret = size;
    lft = size;

    while(lft > 0){
        int status = send(sock, buf, lft, MSG_DONTWAIT|MSG_NOSIGNAL);
        if (status > 0) {
            buf = (char *)buf + status;
            lft -= status;
        } else if(status == 0) {
            ret = 0;
            break;
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN || (errno == EINTR)) {
                continue;
            } else {
                ret = -1;
                break;
            }
        }
    }

    return ret;
}

static int intf_sock_recvn(int sock, void *buf, int size)
{
    int ret, lft;

    ret = size;
    lft = size;

    while (lft > 0) {
        int status = recv(sock, buf, lft, MSG_DONTWAIT|MSG_NOSIGNAL);
        if (status > 0) {
            buf = (char *)buf + status;
            lft -= status;
        } else if(status == 0) {
            ret = size - lft;
            break;
        } else {
            if(errno == EWOULDBLOCK || errno == EAGAIN || (errno == EINTR)){
                continue;
            }else{
                ret = -1;
                break;
            }
        }
    }

    return ret;
}

static char * intf_recv_line(int sock, int size, char *buf)
{
    int   pos  = 0;
    char *line = buf;

    for(pos = 0; pos < size; pos++){
        char c;

        if(1 != intf_sock_recvn(sock, &c, 1)){
            return NULL;
        }

        line[pos] = c;
        if(c == '\n'){
            line[pos] = 0;
            return line;
        }
    }

    return NULL;
}

static int intf_recv_all(int sock, int size, char *buf)
{
    int pos;

    pos = 0;
    while (pos < size) {
        int ret = recv(sock, buf, size - pos, MSG_DONTWAIT|MSG_NOSIGNAL);
        if (ret > 0) {
            buf += ret;
            pos += ret;
        } else if(ret == 0) {
            *buf= 0;
            return pos;
        } else {
            if(errno == EWOULDBLOCK || errno == EAGAIN || (errno == EINTR)){
                continue;
            }else{
                return -1;
            }
        }
    }

    //overflow
    return -1;
}

static int intf_sess_req_parse(fw_intf_sess_t *sess)
{
    int   size = sess->recv_len;
    char *buff = (char *)sess->recv_buf;
    char *line, *ctx;
    int   off  = 5;
    int   line_size;

    if(size < 10){
        return -1;
    }

    /****************************************/
    /* Get request cmd line                 */
    /****************************************/
    if(memcmp(buff, "REQ: ", 5) != 0){
        return -1;
    }
    line = buff + off;

    //Set cmd line terminate
    for(off = 5; off < size; off++){
        if(buff[off] == '\n'){
            buff[off++] = 0;
            break;
        }
    }
    line_size = off - 5;
    if(off > size || line_size > INTF_REQ_LINE){
        return -1;
    }else{
        strncpy(sess->req_line, line, INTF_REQ_LINE);
    }

    //Parse cmd line
    int ac;
    for(ac = 0; ac < INTF_ARG_NMAX; ac++, line = NULL){
        char *tok = strtok_r(line, " &\t", &ctx);

        if(tok){
            sess->req_argv[ac] = tok;
        }else{
            break;
        }
    }
    sess->req_argc = ac;


    /****************************************/
    /* Get request data line                */
    /****************************************/
    if(memcmp(buff+off, "DATA: ", 6) != 0){
        return -1;
    }
    off += 6;
    line = buff + off;

    //Set data line terminate
    for(; off < size; off++){
        if(buff[off] == '\n'){
            buff[off] = 0;
            break;
        }
    }
    if(off > size) return -1;

    //parse data line
    char *num = strtok_r(line, " &\t", &ctx);
    sess->req_data_len = atoi(num);
    sess->req_data     = buff + off;
    if(sess->req_data_len > size - off){
        return -1;
    }

    return 0;
}


static int intf_sess_req_wait(fw_intf_sess_t *sess)
{
    sess->recv_len = intf_recv_all(sess->sock, INTF_BUF_SIZE, (char *)sess->recv_buf);
    if(sess->recv_len < 0){
        return -1;
    }

    if(intf_sess_req_parse(sess) < 0){
        return -1;
    }

    return 0;
}

static int intf_sess_req_proc(fw_intf_sess_t *sess)
{
    if(sess->req_argc > 0){
        sess->resp_result = intf_req_proc (sess);
    }else{
        sess->resp_result = -1;
        sess->resp_len = 11;
        sess->resp_buf = (char *)"Error.empty";
    }

    return 0;
}

static int intf_sess_req_resp(fw_intf_sess_t *sess)
{
    char buff[1024];
    int  off = 0;
    int  lft = 1024;
    int  ret;

    //Set response line header
    if((ret = snprintf(buff + off, lft, "RESP: ")) < 0){
        return -ERR_INTF_LOCAL;
    }else{
        off += ret; lft -= ret;
    }

    //Set response result
    if(sess->resp_result == 0){
        ret = snprintf(buff + off, lft, "OK\n");
    }else{
        ret = snprintf(buff + off, lft, "FAIL\n");
    }
    if(ret < 0){
        return -1;
    }else{
        off += ret; lft -= ret;
    }

    //Set response data line header
    if((ret = snprintf(buff + off, lft, "DATA: %d\n", sess->resp_len)) < 0){
        return -1;
    }else{
        off += ret; lft -= ret;
    }

    //Send response Head
    if(off != intf_sock_sendn(sess->sock, buff, off)){
        return -1;
    }

    //Send response Data
    if(sess->resp_len != intf_sock_sendn(sess->sock, sess->resp_buf, sess->resp_len)){
        return -1;
    }

    return 0;
}

static int intf_sess_proc(fw_intf_sess_t *sess)
{
    if(0 == intf_sess_req_wait(sess)){
        intf_sess_req_proc(sess);
        intf_sess_req_resp(sess);
        return 0;
    }else{
        return -1;
    }
}

static int intf_send_req(int sock, int ac, char **av, int dlen, void *dbuf)
{
    char buff[1024];
    int  off = 0;
    int  lft = 1024;
    int  ret, i;

    if(ac < 1 || av == NULL || (dlen != 0 && dbuf == NULL)){
        return -ERR_INTF_LOCAL;
    }

    //Set request line header
    if((ret = snprintf(buff + off, lft, "REQ: ")) < 0){
        return -ERR_INTF_LOCAL;
    }else{
        off += ret; lft -= ret;
    }

    //Set request args
    for(i = 0; i < ac && av[i]; i++){
        if(i == 0){
            ret = snprintf(buff + off, lft, "%s", av[i]);
        }else{
            ret = snprintf(buff + off, lft, "&%s", av[i]);
        }
        if(ret < 0){
            return -ERR_INTF_LOCAL;
        }else{
            off += ret; lft -= ret;
        }
    }

    //Set request line terminate
    if((ret = snprintf(buff + off, lft, "\n")) < 0){
        return -ERR_INTF_LOCAL;
    }else{
        off += ret; lft -= ret;
    }

    //Set data line header
    if((ret = snprintf(buff + off, lft, "DATA: %d\n", dlen)) < 0){
        return -ERR_INTF_LOCAL;
    }else{
        off += ret; lft -= ret;
    }

    //Send Request Head
    if(off != intf_sock_sendn(sock, buff, off)){
        return -ERR_INTF_TRANS;
    }

    //Send Request Data
    if(dlen != intf_sock_sendn(sock, dbuf, dlen)){
        return -ERR_INTF_TRANS;
    }

    shutdown(sock, SHUT_WR);

    return 0;
}

static int intf_wait_resp(int sock, int *result, int *dlen, void **dbuf)
{
    char  buf1[256], buf2[256];
    char *ctx;
    char *head, *data;
    char *resp, *num;
    char *data_buf;
    int   data_len = 0;

    if(result == NULL){
        return -ERR_INTF_LOCAL;
    }

    if((head = intf_recv_line(sock, 256, buf1)) == NULL){
        return -ERR_INTF_TRANS;
    }

    if((data = intf_recv_line(sock, 256, buf2)) == NULL){
        return -ERR_INTF_TRANS;
    }

    if(memcmp(head, "RESP: ", 6) != 0){
        return -ERR_INTF_REMOTE;
    }else{
        resp = strtok_r(head + 6, " \n", &ctx);
        if(resp == NULL){
            //Syntax error
            return -ERR_INTF_REMOTE;
        }

        if(strcmp(resp, "OK") == 0){
            *result = 0;
        }else{
            *result = -1;
        }
    }

    if(memcmp(data, "DATA: ", 6) != 0){
        //Syntax error
        return -ERR_INTF_REMOTE;
    }else{
        num = strtok_r(data + 6, " \n", &ctx);
        if(num == NULL){
            //Syntax error
            return -ERR_INTF_REMOTE;
        }

        data_len = atoi(num);
        if(data_len < 0){
            return -ERR_INTF_REMOTE;
        }
    }

    if(dlen && dbuf){
        *dlen = 0;
        *dbuf = NULL;
        if(data_len > 0){
            data_buf = (char *) MALLOC(data_len + 1);
            if(data_buf == NULL){
                //Local memory not enought
                return -ERR_INTF_LOCAL;
            }

            if(data_len != intf_sock_recvn(sock, data_buf, data_len)){
                //Translate error
                FREE(data_buf);
                return -ERR_INTF_TRANS;
            }else{
                data_buf[data_len] = 0;
            }
            *dlen = data_len;
            *dbuf = data_buf;
        }
    }

    return 0;
}

/******************************************************************************
 * Global functions                                                         *
 *****************************************************************************/
int fw_intf_init(const char *cmd_addr, const char *cmd_port, fw_intf_req_handle_t cmd_proc)
{
    if(cmd_port == NULL || cmd_proc == NULL){
        return -1;
    }

    //Noused args
    (void)cmd_addr;

    intf_recv_buff = (char *)MALLOC(INTF_BUF_SIZE);
    if(intf_recv_buff == NULL){
        goto DO_ERROR;
    }

    intf_resp_buff = (char *)MALLOC(INTF_BUF_SIZE);
    if(intf_resp_buff == NULL){
        goto DO_ERROR;
    }


    if ((intf_sock = intf_sock_listen(atoi(cmd_port))) < 0) {
        goto DO_ERROR;
    }

    struct timeval      tv;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(intf_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(intf_sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    intf_req_proc = cmd_proc;

    return 0;

DO_ERROR:
    if(intf_recv_buff) FREE(intf_recv_buff);
    if(intf_resp_buff) FREE(intf_resp_buff);

    return -1;
}

int fw_intf_deinit(void)
{
    if(intf_recv_buff) FREE(intf_recv_buff);
    if(intf_resp_buff) FREE(intf_resp_buff);

    return close(intf_sock);
}

int fw_intf_mainloop(void)
{
    while(!intf_done) {
        socklen_t           len = sizeof(struct sockaddr_in);
        struct sockaddr_in  clientaddr;
        int                 sock;
        struct timeval      tv;

        //Wait req session
        if ((sock = accept(intf_sock, (struct sockaddr *)&clientaddr, &len)) < 0) {
            continue;
        }
        //Set sock attr
        tv.tv_sec  = 5;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        //Init req session
        memset(&intf_sess, 0, sizeof(intf_sess));
        intf_sess.sock = sock;
        intf_sess.resp_result = -1;
        intf_sess.resp_buf = intf_resp_buff;
        intf_sess.recv_buf = intf_recv_buff;
        intf_sess_proc(&intf_sess);

        close(sock);
        sock = -1;
    }

    return 0;
}

int fw_intf_exit(void)
{
    intf_done = 1;
    return 0;
}

int fw_intf_request(
    const char *ip, const char *port,
    int argc, char **argv, int dlen, void *dbuf,
    fw_intf_resp_handle_t resp_proc, void *udata)
{
    int   status = -1;
    int   sess_sock;

    int   resp_res = 0;
    int   resp_len = 0;
    void *resp_buf = NULL;

    if((sess_sock = intf_sock_connect(ip, atoi(port))) < 0){
        status = -ERR_INTF_LOCAL;
        goto DO_RETURN;
    }

    if((status = intf_send_req(sess_sock, argc, argv, dlen, dbuf)) != 0){
        goto DO_RETURN;
    }

    status = intf_wait_resp(sess_sock, &resp_res, &resp_len, &resp_buf);

DO_RETURN:
    status = resp_proc(status, resp_res, resp_len, resp_buf, udata);
    if(resp_buf) FREE(resp_buf);

    close(sess_sock);

    return status;
}

#ifdef JIMTCL_EXT
#include <jim.h>

static int
jim_fw_intf_resp_handle(
    int status,
    int resp_result,
    int resp_len,
    void *resp_dat,
    void *udata)
{
    Jim_Interp *interp = udata;
    int   result_len;
    char *result_str;

    if(status == 0 && resp_result != 0){
        status = -ERR_INTF_REMOTE;
    }

    if(result_len > 0){
        result_str = resp_dat;
        result_len = resp_len;
    }else{
        if(status == 0){
            result_str = "Ok";
            result_len = -1;
        }else if(status == -ERR_INTF_REMOTE){
            result_str = "Error.remote";
            result_len = -1;
        }else if(status == -ERR_INTF_TRANS){
            result_str = "Error.translate";
            result_len = -1;
        }else{
            result_str = "Error.local";
            result_len = -1;
        }
    }

    Jim_SetResultString(interp, result_str, result_len);

    return status;
}

static int
jim_fw_intf_Cmd(Jim_Interp *interp, int objc, Jim_Obj *const objv[])
{
    int   retval;
    int   ac;
    char *av[INTF_ARG_NMAX];
    char  target[128];
    char *ip, *port, *ctx;
    int   i;

    if(objc < 3){
        Jim_SetResultString(interp, "Error.Input", -1);
        return JIM_ERR;
    }

    strncpy(target, Jim_String(objv[1]), 128);
    ip   = strtok_r(target, " :", &ctx);
    port = strtok_r(NULL, " ", &ctx);


    for(i = 0; i < objc - 2 && i < INTF_ARG_NMAX; i++){
        av[i] = Jim_String(objv[i+2]);
    }
    ac = i;

    retval = fw_intf_request(
                    ip, port,
                    ac, av, 0, NULL,
                    jim_fw_intf_resp_handle, interp);

    if(retval == 0){
        return JIM_OK;
    }else{
        return JIM_ERR;
    }
}

/*
 * Jim_helloworldInit -- Called when Jim loads your extension.
 *
 * Note that the name *must* correspond exactly to the name of the extension:
 *  Jim_<extname>Init
 */
int
Jim_fw_intfInit(Jim_Interp *interp)
{
    Jim_CreateCommand(interp, "fw_intf", jim_fw_intf_Cmd, NULL, NULL);
    return JIM_OK;
}

#endif

/* End of fw_intf.c */

