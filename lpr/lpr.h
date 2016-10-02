
#ifndef __LPR_H__
#define __LPR_H__
#include "../include/ob_platform.h"
#include "../include/as_protocol.h"

#ifndef MAX_CH
#define MAX_CH		32
#endif

//typedef struct
//{
//	char		plate[32];
//
//	int			x1;
//	int			y1;
//	int			x2;
//	int			y2;
//
//	u32			timestamp;
//	u32			micro_sec;
//
//	int			cout;
//
//	char		image_path[64];
//	//BYTE		imgbuf[76800];
//
//}result_info;

extern void *lpr_alloc(int parking_id, result_info* result, char *filename);
extern void  lpr_free(void *handle);
extern int   lpr_process(void *handle, void *buf, u32 timestamp, u32 micro_sec);
extern int   lpr_dump_result(void *handle, void* priv, void *img_buf);
extern int	 lpr_param_read(void *lpr_handle, int parking_id, char *filename);
extern int   lpr_list_clean(void *handle, int ch_id, int max);
//extern int   lpr_list_sort(int parking_id, char *Retuen_Ans);
#endif // __LPR_H__

