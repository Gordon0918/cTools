#ifndef _SYS_CFG_INC_
#define _SYS_CFG_INC_

#ifdef _cplusplus
extern "C" {
#endif

int sys_cfg_save(char *cfg_name, char *entry_path);
int sys_cfg_load(char *cfg_name);

int sys_cfg_save_mem(int cfg_size, char *cfg_buf, char *entry_path);
int sys_cfg_load_mem(int size, char *cfg_buf);

#ifdef _cplusplus
}
#endif

#endif /* _SYS_CFG_INC_ */
