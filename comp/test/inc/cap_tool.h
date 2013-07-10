/******************************************************************************
* cap_tool.h
*
* author: dinglixing
* 
* 2012.05.31 create
******************************************************************************/

#ifndef _CAP_TOOL_INC_
#define _CAP_TOOL_INC_

/******************************************************************************
* Marco & Const
******************************************************************************/

/******************************************************************************
* types
******************************************************************************/

/******************************************************************************
* variable
******************************************************************************/

/******************************************************************************
* function
******************************************************************************/
int cap_open(const char *name);
int cap_get_packet(int cap, void *buf, int size);
int cap_close(int cap);

#endif /* _CAP_TOOL_INC_ */

/* end of file */

