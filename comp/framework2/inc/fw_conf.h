/**************************************************************************//**
 *
 *       Copyright 2012 GEO RM2000 Team
 *
 * File Name:
 *      fw_conf.h
 * 
 * Brief:
 *      
 * 
 * History:
 *      
 *****************************************************************************/

#ifndef _FW_CONF_INC_
#define _FW_CONF_INC_

/******************************************************************************
 * Types & Define                                                             *
 *****************************************************************************/


/******************************************************************************
 * Global variable declaration                                                *
 *****************************************************************************/


/******************************************************************************
 * Global function declaration                                                *
 *****************************************************************************/
int   fw_conf_init(void);
int   fw_conf_deinit(void);

void *fw_conf_load(const char *conf);
int   fw_conf_save(void *root, const char *conf);
void *fw_conf_node_alloc  (const char *label, const char *content);
void  fw_conf_node_free   (void *node);
void  fw_conf_node_delete (void *node);
char *fw_conf_node_label  (void *node);
char *fw_conf_node_content(void *node);
void *fw_conf_node_parent (void *node);
void *fw_conf_node_child  (void *node, const char *label);
void *fw_conf_node_next   (void *node, const char *label);
void *fw_conf_node_add_child       (void *node, const char *label, const char *content);
void *fw_conf_node_add_child_node  (void *node, void *child_node);
void *fw_conf_node_add_sibling     (void *node, const char *label, const char *content);
void *fw_conf_node_add_sibling_node(void *node, void *sibling_node);
void fw_conf_outprint(void *node);


#endif /* _FW_CONF_INC_ */

/* End of fw_conf.h */

