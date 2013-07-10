#ifndef _CMDCFG_H_
#define _CMDCFG_H_

#define MAXLOG (1024*100)
#define MAXPARAM 16
#define MAXLINE 256

#define CMD_CFG_LOG  "cmd_cfg_log.list"


int cmdcfg_init(char* fname);
char** cmdcfg_getnext(int *argc);

int cmdcfg_add(int argc, char** argv);

int cmdcfg_save(char* fname);

extern char *get_cfgfullname(char *filename);
char *get_cfgdirname();


#endif /*_CMDCFG_H_*/

