#ifndef _DBG_MSG_H_
#define _DBG_MSG_H_


void *Open_Msg(int total_num, char *log_path);
int  Write_Msg(int ch_id, char *msg, int size);
int  Close_Msg(void *file);

#endif