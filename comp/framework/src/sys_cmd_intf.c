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

#include <sys_cmd_intf.h>
#include <sys_usr.h>

#define CMD_MAX         128
#define CMD_ARG_MAX     16
#define CMD_ARG_SIZE    128
#define CMD_BUF_SIZE    (1024 * 128)
#define CMD_CBUF_SIZE   16
#define SESS_NUM    2

enum {
        CMD_REQUEST = 0,
        CMD_LOGIN   = 1,
};

typedef struct _cmd_entry_t {
    const char      *cmd_name;
    sys_cmd_proc_t   cmd_proc;
}cmd_entry_t;

typedef struct _sys_sess_t {
    int           id;
    int           sock;
    time_t        acs;
    sys_usr_t    *usr;
    int           alg;
    unsigned char key[16];
}sys_sess_t;

static int         cmd_cbuf_off = 0;
static char        cmd_cbuf[CMD_CBUF_SIZE];

static char        cmd_buf[CMD_BUF_SIZE];
static char        cmd_arg_buf[CMD_ARG_MAX][CMD_ARG_SIZE];
static int         cmd_parse_flag;
static int         cmd_sess_id;
static int         cmd_result;
static char       *cmd_arg[CMD_ARG_MAX];
static int         cmd_arg_num;
static int         cmd_dat_len;
static char       *cmd_dat_buf;
static int         cmd_sock = -1;

static int         cmd_done = 0;
static int         cmd_num  = 0;
static cmd_entry_t cmd_entrys[CMD_MAX];

static sys_sess_t  sess_def;
static sys_sess_t  sess_tbl[SESS_NUM];
static sys_sess_t *sess_cur = NULL;
static int         sess_id_new = 1;

static unsigned char *sess_key = NULL;

static int sys_sess_init(void)
{
    int i;
    time_t cur;

    cur = time(NULL);
    sess_def.id  = 0;
    sess_def.usr = sys_user_def();
    sess_def.acs = cur;
    sess_def.alg = 0;
    memset(sess_def.key, 0, 16);

    for(i = 0; i < SESS_NUM; i++){
        sys_sess_t *sess = &sess_tbl[i];

        sess->id  = 0;
        sess->alg = 0;
        sess->acs = cur;
        sess->usr = NULL;
        memset(sess->key, 0, 16);
    }

    return 0;
}

static void sys_sess_key_set(unsigned char *key)
{
    sess_key = key;
}

static int sys_sess_set(int id, int sock)
{
    time_t  cur;
    int     i;
    sys_sess_t *sess_match = NULL;
    
    cur = time(NULL);

    if(id == 0){
        sess_match = &sess_def;
    }

    for(i = 0; i < SESS_NUM; i++){
        sys_sess_t *sess = &sess_tbl[i];

        if(sess->id == id){
            sess_match = sess;
            break;
        }
    }

    sess_cur = sess_match;
    if(sess_cur){
        sess_cur = sess_match;
        sess_cur->acs = cur;
        sess_cur->sock= sock;
        sys_sess_key_set(sess_cur->key);
        return 0;
    }else{
        return -1;
    }
}

static sys_sess_t *sys_sess_alloc(sys_usr_t *usr)
{
    time_t cur;
    int     i;
    sys_sess_t *sess_match = NULL;
    
    cur = time(NULL);

    for(i = 0; i < SESS_NUM; i++){
        sys_sess_t *sess = &sess_tbl[i];

        if(sess->usr == NULL){
            sess_match = sess;
            goto DO_RET;
        }
    }

    for(i = 0; i < SESS_NUM; i++){
        sys_sess_t *sess = &sess_tbl[i];

        if(cur - sess->acs > 3600){
            sess_match = sess;
            goto DO_RET;
        }
    }

    for(i = 0; i < SESS_NUM; i++){
        sys_sess_t *sess = &sess_tbl[i];

        if(usr->priority > sess->usr->priority){
            sess_match = sess;
            goto DO_RET;
        }
    }

DO_RET:
    if(sess_match){
        sess_match->id  = sess_id_new++;
        sess_match->acs = cur;
        sess_match->usr = usr;
        memset(sess_match->key, cur, 16);
    }

    return sess_match;
}

static int sys_sess_encrypt(int len, char *buf)
{
    int i;
    
    if(len & 0xf){
        return -1;
    }

    for(i = 0; i < len; i++){
        int k = i & 0xf;

        buf[i] = buf[i] ^ sess_key[k];
    }

    return 0;
}

static int sys_sess_decrypt(int len, char *buf)
{
    int i;
    
    if(len & 0xf){
        return -1;
    }

    for(i = 0; i < len; i++){
        int k = i & 0xf;

        buf[i] = buf[i] ^ sess_key[k];
    }

    return 0;
}

static int sys_cmd_sock(unsigned short port)
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

static int sys_cmd_sendn(int sock, void *buf, int size)
{
    int  ret, lft;

    ret = size;
    lft = size;

    while(lft > 0){
        int status = send(sock, buf, lft, MSG_NOSIGNAL);
        if (status > 0) {
            buf += status;
            lft -= status;
        } else if(status == 0) {
            ret = 0;
            break;
        } else {
            if (errno == EINTR) {
                continue;
            } else {
                ret = -1;
                break;
            }
        }
    }

    return ret;
}

static int sys_cmd_recvn(int sock, void *buf, int size)
{
    int ret, lft;

    ret = size;
    lft = size;

    while (lft > 0) {
        int status = recv(sock, buf, lft, MSG_NOSIGNAL);
        if (status > 0) {
            buf += status;
            lft -= status;
        } else if(status == 0) {
            ret = size - lft;
            break;
        } else {
            if(errno == EINTR){
                continue;
            }else{
                ret = -1;
                break;
            }
        }
    }

    return ret;
}

static int sys_cmd_sendc_init(void)
{
    cmd_cbuf_off = 0;
    return 0;
}

static int sys_cmd_sendc(int sock, int size, char *buf)
{
    int pos;
    int i;
    int len;

    pos = 0;
    while(pos < size){
        for(i = cmd_cbuf_off; i < CMD_CBUF_SIZE && pos < size; i++){
            cmd_cbuf[i] = buf[pos++];
        }

        if(i == CMD_CBUF_SIZE){
            cmd_cbuf_off = 0;
            
            sys_sess_encrypt(CMD_CBUF_SIZE, cmd_cbuf);

            len = sys_cmd_sendn(sock, cmd_cbuf, CMD_CBUF_SIZE);
            if(CMD_CBUF_SIZE != len){
                return -1;
            }
        }else{
            cmd_cbuf_off = i;
        }
    }

    return 0;
}

static int sys_cmd_sendc_final(int sock)
{
    int i;
    int len;

    if(cmd_cbuf_off == 0){
        return 0;
    }
    
    for(i = cmd_cbuf_off; i < CMD_CBUF_SIZE; i++){
        cmd_cbuf[i] = 0;
    }
    cmd_cbuf_off = 0;
    
    sys_sess_encrypt(i, cmd_cbuf);

    len = sys_cmd_sendn(sock, cmd_cbuf, CMD_CBUF_SIZE);
    
    if(CMD_CBUF_SIZE != len){
        return -1;
    }else{
        return 0;
    }
}

static int sys_cmd_default(int ac, char **av, int dat_len, char *dat)
{
    char buf[128];
    int  n;

    n = snprintf(buf, 127, "Unknow cmd: %s", av[0]);
    return sys_cmd_return(-1, n, buf);
}

static int sys_cmd_kill(int ac, char **av, int dat_len, char *dat)
{
    cmd_done = 1;
    
    return sys_cmd_return(0, 0, NULL);
}

static sys_cmd_proc_t sys_cmd_find(char *cmd_name)
{
    int i = 0;

    for (i = 0; i < cmd_num; i++) {
        if (strcmp(cmd_name, cmd_entrys[i].cmd_name) == 0) {
            return cmd_entrys[i].cmd_proc;
        }
    }
    
    return sys_cmd_default;
}



/*
static void sys_cmd_login(int sock)
{
    int  len;
    char cli_key[16];
    char svr_key[16];
    char usr_md5[16];
    char tmp_buf[64];

    //get cli rsa pub_key
    len = sys_cmd_recvn(sock, cli_key, 16);
    if(len != 16){
        return;
    }

    //get svr rsa pub_key
    //encrypt svr rsa pub_key by usr pub_key
    //send svr pub_key
    len = sys_cmd_sendn(sock, svr_key, 16);
    if(len != 16){
        return;
    }

    //get usr uid(md5) encrypt by cli
    len = sys_cmd_recvn(sock, tmp_buf, 16);
    if(len != 16){
        return;
    }
    //decrypt uid by svr_pri_key
    //ras_decrypt(svr_pri_key, tmp_buf, usr_md5);

    //user is invalid
    if(0){
        return;
    }

    //alloc sess
    //gen trans key
    //encrypt sess id & trans key
    memset(tmp_buf, 0, 64);
    len = snprintf(tmp_buf, 64 - 16, "SESS:%d,ALG:%s,LEN\n",
                                      sess_id, alg);
    memxpy(tmp_buf + len, trans_key, 16);

    len += 16;
    //rsa_encrypt(cli_pub_key, tmp_buf);
    //send;

    //end;
}
*/

static char * sys_cmd_recv_line(int sock, int size, char *buf)
{
    int   pos  = 0;
    char *line = buf;

    for(pos = 0; pos < size; pos++){
        char c;
        
        if(1 != sys_cmd_recvn(sock, &c, 1)){
            return NULL;
        }
        buf[pos] = c;
        if(c == '\n'){
            buf[pos] = 0;
            return line;
        }
    }

    return NULL;
}

static int sys_cmd_recv_all(int sock, int size, char *buf)
{
    int pos;
    char *line;

    line = sys_cmd_recv_line(sock, size, buf);
    if(line == NULL){
        return -1;
    }

    if(memcmp(line, "LOGIN",5) == 0){
        //means login req
        return 0;
    }

    if(memcmp(line, "SESS: ", 6) != 0){
        return -1;
    }else{
        char *p;
        char *id_str;

        id_str = strtok_r(&line[6], " \n", &p);
        if(NULL == id_str){
            return -1;
        }else{
            cmd_sess_id = atoi(id_str);
        }
    }

    pos = 0;
    while (pos < size) {
        int ret = recv(sock, buf, size - pos, MSG_NOSIGNAL);
        if (ret > 0) {
            buf += ret;
            pos += ret;
        } else if(ret == 0) {
            *buf= 0;
            return pos;
        } else {
            if(errno == EINTR){
                continue;
            }else{
                return -1;
            }
        }
    }
    //overflow
    return -1;
}

static char * sys_cmd_line_get(int len, int *pos)
{
    int   off;
    char *line;

    off  = *pos;
    line = &cmd_buf[off];
    
    while(off < len){
        if(cmd_buf[off] == '\n'){
            cmd_buf[off++] = 0;
            *pos = off;
            return line;
        }
        off++;
    }

    return NULL;
}

static int sys_cmd_data_get(int len, int pos)
{
    int dat_len = len - pos;

    if(cmd_dat_len > dat_len){
        return -1;
    }else{
        cmd_dat_len = dat_len;
    }

    cmd_dat_buf = &cmd_buf[pos];

    return 0;
}

static void sys_cmd_parse_init(void)
{
    int i;
    
    cmd_arg_num = 0;
    for(i = 0; i < CMD_ARG_MAX; i++){
        cmd_arg[i] = cmd_arg_buf[i];
    }
    
    cmd_result  = 0;
    cmd_dat_len = 0;
    cmd_dat_buf = NULL;
    cmd_parse_flag = 0;
}

static int sys_cmd_parse_line(char *line)
{
    char  c0, c2;
    char *pos;

    c0 = line[0];
    c2 = line[2];
    
    switch(c0){
        case 'R': 
            if(c2 == 'S'){
                goto PARSE_RESP; 
            }else
            if(c2 == 'Q'){
                goto PARSE_REQ;
            }else{
                return -1;
            }
        case 'D': goto PARSE_DAT;
        default : return -1;
    }

PARSE_REQ:
    if(memcmp("REQ: ", line, 5) != 0){
        return -1;
    }
    pos = &line[5];
    while(*pos == ' ') pos++;

    char *term = pos;
    while(*term){
        c0 = *term;
        if(c0 == '&'){
            char *arg_buf = cmd_arg_buf[cmd_arg_num++];

            *term++ = 0;
            strncpy(arg_buf, pos, CMD_ARG_SIZE);
            pos = term;
        }else{
            term++;
        }
    }
    //add the last arg
    if(pos != term){
        char *arg_buf = cmd_arg_buf[cmd_arg_num++];
        
        strncpy(arg_buf, pos, CMD_ARG_SIZE);
    }

    cmd_parse_flag ^= 1;
    //printf("req ok\n");
    return 0;

PARSE_RESP:
    if(memcmp("RESP: ", line, 6) != 0){
        return -1;
    }

    if(memcmp("OK", &line[6], 2) == 0){
        cmd_result = 0;
    }else
    if(memcmp("FAIL", &line[6], 4) == 0){
        cmd_result = -1;
    }else{
        return -1;
    }
    
    cmd_parse_flag ^= 4;
    //printf("resp ok\n");
    return 0;

PARSE_DAT:
    if(memcmp("DATA: ", line, 6) != 0){
        return -1;
    }
    pos = &line[6];
    while(*pos == ' ') pos++;

    cmd_dat_len = atoi(pos);

    cmd_parse_flag ^= 2;
    //printf("data ok\n");
    return 0;
}

static int sys_cmd_req_parse_ok(void)
{
    return cmd_parse_flag == 0x3;
}

static int sys_cmd_resp_parse_ok(void)
{
    return cmd_parse_flag == 0x6;
}

static int sys_cmd_wait(int sock)
{
    int   cmd_len;
    char *line;
    int   pos;

    cmd_len = sys_cmd_recv_all(sock, CMD_BUF_SIZE, cmd_buf);
    if(cmd_len < 0){
        return -1;
    }
    if(cmd_len == 0){
        return CMD_LOGIN;
    }

    if(0 != sys_sess_set(cmd_sess_id, sock)){
        return -1;
    }
    sys_sess_decrypt(cmd_len, cmd_buf);
    sys_cmd_parse_init();
    pos = 0;
    while(NULL != (line = sys_cmd_line_get(cmd_len, &pos))){
        if(0 != sys_cmd_parse_line(line)){
            return -1;
        }
        if(sys_cmd_req_parse_ok()){
            break;
        }
    }
    if(0 != sys_cmd_data_get(cmd_len, pos)){
        return -1;
    }
    return CMD_REQUEST;
}

static int sys_cmd_proc(void)
{
    sys_cmd_proc_t cmd_func;

    cmd_func = sys_cmd_find(cmd_arg[0]);
    
    return cmd_func(cmd_arg_num, cmd_arg, cmd_dat_len, cmd_dat_buf);
}

static int sys_cmd_login(int sock)
{
    int  len;
    char tmp_buf[64];
    sys_usr_t  *usr;
    sys_sess_t *sess;

    if(NULL == sys_cmd_recv_line(sock, 64, tmp_buf)){
        len = snprintf(tmp_buf, 64, "FAIL: translate");
        goto DO_RESP;
    }

    usr = sys_user_find(tmp_buf);
    if(NULL == usr){
        len = snprintf(tmp_buf, 64, "FAIL: Invalid usr or passwd");
        goto DO_RESP;
    }

    sess = sys_sess_alloc(usr);
    if(NULL == sess){
        len = snprintf(tmp_buf, 64, "FAIL: sess busy!");
    }else{
        len = snprintf(tmp_buf, 64, "OK:%d\n", sess->id);
    }

DO_RESP:
    sys_cmd_sendn(sock, tmp_buf, len);
    if(sess){
        sys_cmd_sendn(sock, sess->key, 16);
    }
    
    return 0;
}

int sys_cmd_register(const char *cmd_name, sys_cmd_proc_t cmd_proc)
{
    cmd_entry_t *pcmd;
    
    if(cmd_num >= CMD_MAX){
        return -1;
    }

    pcmd = &cmd_entrys[cmd_num++];

    pcmd->cmd_name = cmd_name;
    pcmd->cmd_proc = cmd_proc;
    
    return 0;
}

int sys_cmd_init(const char *cmd_addr, unsigned short cmd_port)
{
    sys_sess_init();
    
    if ((cmd_sock = sys_cmd_sock(cmd_port)) < 0) {
        return -1;
    }else{
        return 0;
    }
}

int sys_cmd_deinit(void)
{
    return close(cmd_sock);
}

int sys_cmd_mainloop(void)
{
    while(!cmd_done) {
        int                 cmd_type;
        socklen_t           len = sizeof(struct sockaddr_in);
        struct sockaddr_in  clientaddr;
        int                 sess_sock;
        struct timeval      tv;

        if ((sess_sock = accept(cmd_sock, (struct sockaddr *)&clientaddr, &len)) < 0) {
            continue;
        }

        tv.tv_sec  = 5;
        tv.tv_usec = 0;

        setsockopt(sess_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sess_sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        cmd_type = sys_cmd_wait(sess_sock);
        if(cmd_type == CMD_REQUEST){
            sys_cmd_proc();
        }else if(cmd_type == CMD_LOGIN){
            sys_cmd_login(sess_sock);
        }else{
            //
        }    
        
        close(sess_sock);
    }
    
    return 0;
}

int sys_cmd_return(int ret_val, int ret_size, char *ret_buf)
{
    char *buf;
    int  len;
    int  ret;
    int  off;
    int  lft;
    int  sock;
    
    sock = sess_cur->sock;

    len = snprintf(cmd_buf, 1024, "SESS: %d\n", cmd_sess_id);
    ret = sys_cmd_sendn(sock, cmd_buf, len);
    if(ret != len){
        return -1;
    }

    off = 0;
    lft = CMD_BUF_SIZE;    
    buf = cmd_buf;
    len = snprintf(buf, lft, "RESP: %s\nDATA: %u\n", 
                             ret_val ? "FAIL" : "OK",
                             ret_size);
    off += len;
    lft -= len;
    buf += len;

    if(lft < ret_size){
        return -1;
    }

    memcpy(buf, ret_buf, ret_size);
    off += ret_size;
    sys_cmd_sendc_init();
    if(0 != sys_cmd_sendc(sock, off, cmd_buf)){
        return -1;
    }
    if(0 != sys_cmd_sendc_final(sock)){
        return -1;
    }
    
    return 0;
}

int sys_cmd_return_dync(int ret_val, int (*gen_ret)(void *, int, char *), void *uarg)
{
    char *buf;
    int  len;
    int  ret;
    int  off;
    int  lft;
    int  sock;

    sock = sess_cur->sock;
    
    len = snprintf(cmd_buf, 1024, "SESS: %d\n", cmd_sess_id);
    ret = sys_cmd_sendn(sock, cmd_buf, len);
    if(ret != len){
        return -1;
    }

    off = 0;
    lft = CMD_BUF_SIZE;    
    buf = cmd_buf;
    len = snprintf(buf, lft, "RESP: %s\nDATA: ?\n", 
                             ret_val ? "FAIL" : "OK");
    off += len;
    sys_cmd_sendc_init();
    if(0 != sys_cmd_sendc(sock, off, cmd_buf)){
        return -1;
    }
    while((len = gen_ret(uarg, CMD_BUF_SIZE, cmd_buf)) > 0){
        if(0 != sys_cmd_sendc(sock, len, cmd_buf)){
            return -1;
        }
    }
    if(0 != sys_cmd_sendc_final(sock)){
        return -1;
    }

    return 0;
}

int sys_cmd_install(void)
{
int sys_cmd_list(int, char **, int, char *);
int sys_cmd_info(int, char **, int, char *);
int sys_cmd_set (int, char **, int, char *);
int sys_cmd_get (int, char **, int, char *);
int sys_cmd_clr (int, char **, int, char *);

int sys_cmd_load(int, char **, int, char *);
int sys_cmd_save(int, char **, int, char *);

    sys_cmd_register("kill", sys_cmd_kill);
    sys_cmd_register("list", sys_cmd_list);
    sys_cmd_register("info", sys_cmd_info);
    sys_cmd_register("set",  sys_cmd_set);
    sys_cmd_register("get",  sys_cmd_get);
    sys_cmd_register("clr",  sys_cmd_clr);

    sys_cmd_register("save",  sys_cmd_save);
    sys_cmd_register("load",  sys_cmd_load);

    return 0;
}

static struct sockaddr_in   rcall_svr_addr;
static int                  rcall_sess_id = 0;
static unsigned char        rcall_key[16];

static int sys_cmd_rcall_sock(void)
{
    int             sock;
    struct timeval  tv;
    
    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    
    if ( connect(sock, (struct sockaddr*)&rcall_svr_addr,
                sizeof(rcall_svr_addr)) != 0) {
        close(sock);
        return -1;
    }

    tv.tv_sec  = 5;
    tv.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));


    return sock;
}

static int sys_cmd_rcall_req(int sock, int ac, char **av, int dat_len, char *dat_buf)
{
    char *buf;
    int  len;
    int  ret;
    int  off;
    int  lft;
    int  i;

    /*send session*/
    len = snprintf(cmd_buf, sizeof(cmd_buf), "SESS: %d\n", rcall_sess_id);
    ret = sys_cmd_sendn(sock, cmd_buf, len);
    if(ret != len){
        return -1;
    }
    /*compose request string, need to check buffer length*/
    off = 0;
    lft = CMD_BUF_SIZE;    
    buf = &cmd_buf[off];

    len = snprintf(buf, lft, "REQ: ");
    off += len;
    lft -= len;
    buf  = &cmd_buf[off];
    
    for(i = 0; i < ac; i++){
        if(i == 0){
            len = snprintf(buf, lft, "%s", av[i]);
        }else{
            len = snprintf(buf, lft, "&%s", av[i]);
        }
        off += len;
        lft -= len;
        buf  = &cmd_buf[off];
    }
    off += 1;
    lft -= 1;
    *buf++  = '\n';

    len = snprintf(buf, lft, "DATA: %d\n", dat_len);
    off += len;
    lft -= len;
    buf += len;
    
    if(lft < dat_len){
        return -1;
    }

    if(dat_len > 0){
        memcpy(buf, dat_buf, dat_len);
        off += dat_len;
    }
    
    cmd_buf[off] = 0;
    
    sys_cmd_sendc_init();
    if(0 != sys_cmd_sendc(sock, off, cmd_buf)){
        return -1;
    }

    if(0 != sys_cmd_sendc_final(sock)){
        return -1;
    }
    shutdown(sock, SHUT_WR);
    
    return 0;
}

static int sys_cmd_racll_resp_wait(int sock)
{
    int   cmd_len;
    char *line;
    int   pos;

    cmd_len = sys_cmd_recv_all(sock, CMD_BUF_SIZE, cmd_buf);
    if(cmd_len <= 0){
        return -1;
    }

    if(cmd_sess_id != rcall_sess_id){
        return -1;
    }
    sys_sess_decrypt(cmd_len, cmd_buf);
    sys_cmd_parse_init();
    pos = 0;
    while(NULL != (line = sys_cmd_line_get(cmd_len, &pos))){
        if(0 != sys_cmd_parse_line(line)){
            return -1;
        }
        
        if(sys_cmd_resp_parse_ok()){
            break;
        }
    }
    if(0 != sys_cmd_data_get(cmd_len, pos)){
        return -1;
    }

    return 0;
}

static int sys_cmd_rcall_resp(int sock, int *ret_len, char **ret_buf)
{
    if(0 != sys_cmd_racll_resp_wait(sock)){
        return -1;
    }

    if(ret_len) *ret_len = cmd_dat_len;
    if(ret_buf) *ret_buf = cmd_dat_buf;
    
    return cmd_result;
}

int sys_cmd_rcall_init(const char *svr_ip, unsigned short svr_port)
{
    memset(&rcall_svr_addr, 0, sizeof(rcall_svr_addr));
    rcall_svr_addr.sin_family = AF_INET;
    rcall_svr_addr.sin_port   = htons(svr_port);
    
    if ( inet_pton(AF_INET, svr_ip, &rcall_svr_addr.sin_addr) <= 0) {
        return -1;
    }

    memset(rcall_key, 0, sizeof(rcall_key));
    sys_sess_key_set(rcall_key);

    return 0;
}

int sys_cmd_rcall(int ac, char **av, int dat_len, char *dat_buf, int *ret_len, char **ret_buf)
{
    int sock;
    int ret_val;

    sock = sys_cmd_rcall_sock();
    if(sock < 0){
        return -1;
    }

    if(0 != sys_cmd_rcall_req(sock, ac, av, dat_len, dat_buf)){
        return -1;
    }

    ret_val = sys_cmd_rcall_resp(sock, ret_len, ret_buf);

    close(sock);

    return ret_val;
}

//Just a joy!!!!
int sys_cmd_in_security(const char *user, const char *passwd)
{
    int  len;
    int  ret;
    char tmp_buf[64];
    int  sock;
    
    sock = sys_cmd_rcall_sock();
    if(sock < 0){
        return -1;
    }

    len = snprintf(tmp_buf, 64, "%s.%s\n", user, passwd);
    ret = sys_cmd_sendn(sock, tmp_buf, len);
    if(ret != len){
        return -1;
    }

    if(NULL == sys_cmd_recv_line(sock, 64, tmp_buf)){
        return -1;
    }
    if(memcmp(tmp_buf, "OK:", 3) != 0){
        printf("%s\n", tmp_buf);
        return -1;
    }

    rcall_sess_id = atoi(&tmp_buf[3]);
    if(rcall_sess_id == 0){
        return -1;
    }
    
    ret = sys_cmd_recvn(sock, rcall_key, 16);
    if(ret != 16){
        return -1;
    }

    return 0;
}

int sys_cmd_out_security(void)
{
    //rcall!
    memset(rcall_key, 0, 16);
    rcall_sess_id = 0;

    return 0;
}

