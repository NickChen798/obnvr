#ifndef _DBG_LOG_H_
#define _DBG_LOG_H_


void *Open_Log(int total_num, char *log_path);
int  Write_Log(int ch_id, char *msg);
int  Close_Log(void *file);

#endif