
#ifndef __OB_RECORDER_H__
#define __OB_RECORDER_H__

#include "ob_platform.h"
#include "as_protocol.h"


extern int rec_init(rec_config_t *rc);
extern int rec_exit(void);
extern void rec_file_time(u32 sec, u32 *start, u32 *duration);
extern int rec_get_filename(int id, u32 ts, char *name, int ssize);
extern void rec_next_timeslot(u32 sec, u32 *start, u32 *end);
extern void rec_prev_timeslot(u32 sec, u32 *start, u32 *end);
extern int rec_get_time(u32 *min, u32 *max);
extern int Insert_rec_list(int vs1_index, int ch_num);

extern int rec_get_plate_filename(u32 ts, char *name, int ssize);
extern int rec_get_vloss_filename(u32 ts, char *name, int ssize);
extern int rec_event_plate(u8 vs1_ch, u8 id, char *plate, u32 sec, u32 usec, u16 x, u16 y, u16 w, u16 h);
extern int rec_event_video(u8 ch, int vloss);

//extern int _GrayMotionRate( HANDLE hGray1, int nWidth1, HANDLE hGray2, int nWidth2, int nHeight, LPRECT lpRect);

#endif // __OB_RECORDER_H__

