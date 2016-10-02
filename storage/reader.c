/*!
 *  @file       reader.c
 *  @version    1.0
 *  @date       05/05/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       Reader
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "ob_platform.h"
#include "ob_reader.h"
#include "ob_file.h"
#include "ob_event.h"
#include "as_protocol.h"
#include "ob_recorder.h"
#include "codec/codec.h"
#include "dbg_msg/dbg_log.h"


// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "RD"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------

#define READER_MAGIC    0x7244724d      // 'rDrM'
#define BSIZE           (1920*1080)
typedef struct {

    u32     magic;

    int     ch;
    int     id;
    time_t  start;
    time_t  end;
    int     event_type;
    int     image_type;
    int     order;
    u32     x;
    u32     y;
    u32     width;
    u32     height;

    void    *v_handle;
    void    *e_handle;
    time_t  current_time;
    as_frame_t fh;
    ob_event_t ev;

    void    *buf;
    void    *buf2;

} reader_t;

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------

//static char				log_msg[256];

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
static int time_shift(reader_t *r)
{
    u32 start, end;
    u32 min, max;
    rec_get_time(&min, &max);

    if(r->order == OBR_ORDER_DESCENT) {
        if(r->current_time <= r->start)
            return -1;
        rec_prev_timeslot(r->current_time, &start, &end);
        if(end < min)
            return -1;
        if(end < r->current_time)
            r->current_time = end;
    }
    else {
        if(r->current_time >= r->end)
            return -1;
        rec_next_timeslot(r->current_time, &start, &end);
        if(start > max)
            return -1;
        if(start > r->current_time)
            r->current_time = start;
    }

    return 0;
}

static int open_vfile(reader_t *r)
{
    char name[128];
	char				log_msg[256];
    while(r->v_handle == 0) {
        rec_get_filename(r->ch, r->current_time, name, 128);
        //ob_debug("#%d : %u in [%u-%u], %s\n", r->ch, (u32)r->current_time, (u32)r->start, (u32)r->end, name);
        r->v_handle = obf_open(name, 1, 0, 0);
        if(r->v_handle) {
            if(r->order == OBR_ORDER_DESCENT)
                obf_set_dir(r->v_handle, 1);
            if(obf_seek(r->v_handle, r->current_time) == 0)
                break;
            //ob_debug("#%d : %u in [%u-%u], seek failed\n", r->ch, (u32)r->current_time, (u32)r->start, (u32)r->end);
            sprintf(log_msg,"[RD] #%d : %u in [%u-%u], seek failed\n", r->ch, (u32)r->current_time, (u32)r->start, (u32)r->end);
			Write_Log(0,log_msg);
			obf_close(r->v_handle);
            r->v_handle = 0;
        }
        else {
            //ob_debug("#%d : %s open failed\n", r->ch, name);
        }
        if(time_shift(r) < 0) {
            //ob_error("#%d: no valid file @ %u, [%u-%u]\n", r->ch, (u32)r->current_time, (u32)r->start, (u32)r->end);
            sprintf(log_msg,"[RD] #%d: no valid file @ %u, [%u-%u]\n", r->ch, (u32)r->current_time, (u32)r->start, (u32)r->end);            
			Write_Log(0,log_msg);
			return -1;
        }
        else {
            //ob_error("#%d: try %u, [%u-%u]\n", r->ch, (u32)r->current_time, (u32)r->start, (u32)r->end);
		}
    }

    //ob_debug("#%d @ %u : %s opened\n", r->ch, (u32)r->current_time, name);
	sprintf(log_msg,"[RD] #%d @ %u : %s opened\n", r->ch, (u32)r->current_time, name);            
	Write_Log(0,log_msg);
    return 0;
}

static int open_event(reader_t *r)
{
    char name[128];
	char				log_msg[256];
    while(r->e_handle == 0) {
        if(r->event_type == OBR_EV_PLATE)
            rec_get_plate_filename(r->current_time, name, 128);
        else
            rec_get_vloss_filename(r->current_time, name, 128);
        //ob_debug("event: %u in [%u-%u], %s\n", (u32)r->current_time, (u32)r->start, (u32)r->end, name);
        r->e_handle = obe_open(name, 1, 0, 0);
        if(r->e_handle) {
            if(r->order == OBR_ORDER_DESCENT)
                obe_set_dir(r->e_handle, 1);
            if(obe_seek(r->e_handle, r->current_time) == 0)
                break;
            //ob_debug("event: %u in [%u-%u], seek failed\n", (u32)r->current_time, (u32)r->start, (u32)r->end);
            sprintf(log_msg,"[RD] event: %u in [%u-%u], seek failed\n", (u32)r->current_time, (u32)r->start, (u32)r->end);            
			Write_Log(0,log_msg);
			obf_close(r->e_handle);
            r->e_handle = 0;
        }
        else {
            //ob_debug("event: %s open failed\n", name);
        }
        if(time_shift(r) < 0) {
            //ob_error("event: no valid file @ %u, [%u-%u]\n", (u32)r->current_time, (u32)r->start, (u32)r->end);
			sprintf(log_msg,"[RD] event: no valid file @ %u, [%u-%u]\n", (u32)r->current_time, (u32)r->start, (u32)r->end);
			Write_Log(0,log_msg);
			return -1;
        }
        else {
            //ob_error("event: try %u, [%u-%u]\n", (u32)r->current_time, (u32)r->start, (u32)r->end);
        }
    }

    //ob_debug("event @ %u : %s opened\n", (u32)r->current_time, name);
	sprintf(log_msg,"[RD] event @ %u : %s opened\n", (u32)r->current_time, name);
	Write_Log(0,log_msg);
    return 0;
}

static int do_snapshot(reader_t *r, int ch, u32 ts)
{
    char name[128];
    u32 min = ts - 5;
    u32 max = ts + 5;
    u32 start, end;
	char				log_msg[256];
    if(r->v_handle) {
        obf_close(r->v_handle);
        r->v_handle = 0;
    }

    while(r->v_handle == 0) {

        rec_get_filename(ch, ts, name, 128);
        //ob_debug("#%d : %u in %s\n", ch, ts, name);
        r->v_handle = obf_open(name, 1, 0, 0);
        if(r->v_handle) {
            if(r->order == OBR_ORDER_DESCENT)
                obf_set_dir(r->v_handle, 1);
            if(obf_seek(r->v_handle, ts) == 0) {
                if(obf_read(r->v_handle, &r->fh, r->buf, BSIZE) == 0) {
                    obf_close(r->v_handle);
                    r->v_handle = 0;
                    return 0;
                }
                //ob_debug("%s: read failed\n", name);
				sprintf(log_msg,"[RD] %s: read failed\n", name);
				Write_Log(0,log_msg);
            }
            else
                //ob_debug("%s : seek to %u failed\n", name, ts);
				sprintf(log_msg,"[RD] %s : seek to %u failed\n", name, ts);
				Write_Log(0,log_msg);
            obf_close(r->v_handle);
            r->v_handle = 0;
        }
        else {
            //ob_debug("%s open failed\n", name);
        }
        if(r->order == OBR_ORDER_DESCENT) {
            rec_prev_timeslot(ts, &start, &end);
            if(end < min)
                return -1;
            if(end < ts)
                ts = end;
        }
        else {
            rec_next_timeslot(ts, &start, &end);
            if(start > max)
                return -1;
            if(ts < start)
                ts = start;
        }
    }

    return -1;
}

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------
void *obr_alloc(int vs1_ch, int id, time_t start, time_t end)
{
	char				log_msg[256];
    if(start > end)
        return 0;
    reader_t *r = (reader_t *)malloc(sizeof(reader_t));
    if(!r)
        return 0;
    memset(r, 0, sizeof(reader_t));
    u32 min, max;
    if(rec_get_time(&min, &max) < 0) {
        r->start = time(0);
        r->end = r->start + 1;
    }
    else {
        r->start = (start < min) ? min : start;
        r->end = (end > max) ? max : end;
    }
    r->ch = vs1_ch;
    r->id = id;
    r->magic = READER_MAGIC;
    r->buf = malloc(BSIZE);
	if (!r->buf)
	{
		sprintf(log_msg,"[RD] malloc buf: [%u-%u] => [%u-%u]\n", (u32)start, (u32)end, (u32)r->start, (u32)r->end);
		Write_Log(0,log_msg);
	}

    r->buf2 = malloc(BSIZE);

	if (!r->buf2)
	{
		sprintf(log_msg,"[RD] malloc buf2: [%u-%u] => [%u-%u]\n", (u32)start, (u32)end, (u32)r->start, (u32)r->end);
		Write_Log(0,log_msg);
	}
	//ob_debug("obr_alloc: [%u-%u] => [%u-%u]\n", (u32)start, (u32)end, (u32)r->start, (u32)r->end);
	sprintf(log_msg,"[RD] obr_alloc: [%u-%u] => [%u-%u]\n", (u32)start, (u32)end, (u32)r->start, (u32)r->end);
	Write_Log(0,log_msg);
    return r;
}

int obr_free(void *handle)
{
    reader_t *r = (reader_t *)handle;
    if(!r || r->magic != READER_MAGIC)
        return -1;
    if(r->v_handle) {
        obf_close(r->v_handle);
        r->v_handle = 0;
    }
    if(r->e_handle) {
        obe_close(r->e_handle);
        r->e_handle = 0;
    }
    if(r->buf) {
        free(r->buf);
        r->buf = 0;
    }
    if(r->buf2) {
        free(r->buf2);
        r->buf2 = 0;
    }
    r->magic = 0;
    free(r);
    return 0;
}

int obr_setup(void *handle, int event_type, int image_type, int order)
{
    reader_t *r = (reader_t *)handle;
    if(!r || r->magic != READER_MAGIC)
        return -1;
    if(event_type < OBR_EV_PLATE || event_type > OBR_EV_VIDEO)
        return -1;
    if(image_type < OBR_IMG_NONE || image_type > OBR_IMG_PLATE)
        return -1;
    r->event_type = event_type;
    r->image_type = image_type;
    r->order = (order) ? OBR_ORDER_DESCENT : OBR_ORDER_ASCENT;
    r->current_time = (order) ? r->end : r->start;
    return 0;
}

int obr_read(void *handle, obr_result_t *result)
{
	char				log_msg[512];
    reader_t *r = (reader_t *)handle;
    if(!r || r->magic != READER_MAGIC || !result)
	{
		//printf("[1]");
		Write_Log(0,"[RD] [VIDEO] State1");
		return -1;
	}
    memset(result, 0, sizeof(obr_result_t));

    if(r->event_type == OBR_EV_VIDEO) {
		sprintf(log_msg,"[RD] [VIDEO] Begin ch = %d, id = %d, start = %d, end = %d, current_time = %d, sec = %d, usec = %d, \n",r->ch,r->id,r->start,r->end,r->current_time,r->fh.sec,r->fh.usec);
        Write_Log(0,log_msg);
		if(!r->buf)
		{
			//printf("[2]");
			Write_Log(0,"[RD] [VIDEO] State2");
			return -2;
		}
        while(1) {
            if(!r->v_handle) {
                if(open_vfile(r) < 0)
				{
					//printf("[3]");
					Write_Log(0,"[RD] [VIDEO] State3");
					return -3;
				}
            }
			sprintf(log_msg,"[RD] [VIDEO] Open_file  ch = %d, id = %d, start = %d, end = %d, current_time = %d, sec = %d, usec = %d, \n",r->ch,r->id,r->start,r->end,r->current_time,r->fh.sec,r->fh.usec);
			Write_Log(0,log_msg);
            if(r->order == OBR_ORDER_DESCENT) {
                if(r->current_time < r->start)
				{
					//printf("[4]");
					Write_Log(0,"[RD] [VIDEO] State4");
					return -4;
				}
            }
            if(obf_read(r->v_handle, &r->fh, r->buf, BSIZE) == 0)
                break;
            obf_close(r->v_handle);
            r->v_handle = 0;
        }

        result->ch = r->ch;
        result->buffer = r->buf;
        result->size = r->fh.size - r->fh.padding;
        result->data_type = (r->fh.sub_type == ASF_VTYPE_H264) ?  OBR_DT_H264 : OBR_DT_JPEG;
        result->width = r->fh.width;
        result->height = r->fh.height;
        result->sec = r->fh.sec;
        result->usec = r->fh.usec;
		sprintf(log_msg,"[RD] [VIDEO] Result ch = %d, size = %d, data_type = %d, width = %d, height = %d, sec = %d, usec = %d, \n",result->ch,result->size,result->data_type,result->width,result->height,result->sec,result->usec);
		Write_Log(0,log_msg);
        if(r->order == OBR_ORDER_DESCENT) {
            r->current_time = r->fh.sec - 1;
            if(r->fh.sec < r->start)
			{
				//printf("[5]");
				sprintf(log_msg,"[RD] [VIDEO] State5 ch = %d, id = %d, start = %d, end = %d, current_time = %d, sec = %d, usec = %d, \n",r->ch,r->id,r->start,r->end,r->current_time,r->fh.sec,r->fh.usec);
				Write_Log(0,log_msg);
				return -5;
			}
        }
        else {
            r->current_time = r->fh.sec + 1;
            if(r->fh.sec > r->end)
			{
				//printf("[6]");
				sprintf(log_msg,"[RD] [VIDEO] State6 ch = %d, id = %d, start = %d, end = %d, current_time = %d, sec = %d, usec = %d, \n",r->ch,r->id,r->start,r->end,r->current_time,r->fh.sec,r->fh.usec);
				Write_Log(0,log_msg);
				return -6;
			}
        }
    }
    else {
        u8 ch = (r->ch < 0) ? OBE_CH_ALL : (r->ch & 0xff);
        u8 et = (r->event_type == OBR_EV_PLATE) ? OBE_TYPE_PLATE :
                                                  ((r->event_type == OBR_EV_VLOSS) ? OBE_TYPE_VLOSS :
                                                                                     OBE_TYPE_VRECOVERY);
        while(1) {
            if(!r->e_handle) {
                if(open_event(r) < 0)
				{
					//printf("[7]");
					Write_Log(0,"[RD] [VIDEO] State7");
					return -7;
				}
            }
            if(r->order == OBR_ORDER_DESCENT) {
                if(r->current_time < r->start)
				{
					//printf("[8]");
					Write_Log(0,"[RD] [VIDEO] State8");
					return -8;
				}
            }
            if(obe_read(r->e_handle, ch, r->id, et, &r->ev) == 0)
                break;

            if(time_shift(r) < 0) {
                //ob_error("event: no valid file @ %u, [%u-%u]\n", (u32)r->current_time, (u32)r->start, (u32)r->end);
                sprintf(log_msg,"[RD] event: no valid file @ %u, [%u-%u]\n", (u32)r->current_time, (u32)r->start, (u32)r->end);
				Write_Log(0,log_msg);
				return -1;
            }
            obf_close(r->e_handle);
            r->e_handle = 0;
        }

        result->ch = r->ev.ch;
        result->id = r->ev.id;
        result->sec = r->ev.sec;
        result->usec = r->ev.usec;
        r->current_time = r->ev.sec;
        if(r->order == OBR_ORDER_DESCENT) {
            if(r->ev.sec < r->start)
			{
				//sprintf(log_msg,"[9] ev.sec=%d , r.start=%d. \n", r->ev.sec, r->start);
				Write_Log(0,"[RD] [VIDEO] State9");
				return -9;
			}
        }
        else {
            if(r->ev.sec > r->end)
			{
				//printf("[10]");
				Write_Log(0,"[RD] [VIDEO] State10");
				return -10;
			}
        }

        if(r->event_type == OBR_EV_PLATE) {
            strncpy(result->plate, r->ev.plate, OBR_PLATE_LEN);
        }
        if(r->image_type != OBR_IMG_NONE) {
            if(do_snapshot(r, result->ch, r->current_time) == 0) {
                u16 x,y,w,h;
                if(r->image_type == OBR_IMG_FULL || r->fh.sub_type != ASF_VTYPE_H264 || r->ev.width == 0 || r->ev.height == 0) {
                    x = y = 0;
                    w = r->fh.width;
                    h = r->fh.height;
                }
                else {
                    x = r->ev.x;
                    y = r->ev.y;
                    w = r->ev.width;
                    h = r->ev.height;
                    if(w > r->fh.width)
                        w = r->fh.width;
                    if(x > r->fh.width || (x + w) > r->fh.width)
                        x = r->fh.width - w;
                    if(h > r->fh.height)
                        h = r->fh.height;
                    if(y > r->fh.height || (y + h) > r->fh.height)
                        y = r->fh.height - h;
                }
                if(r->fh.sub_type == ASF_VTYPE_H264) {
                    result->size = h264tojpeg(r->buf, r->fh.size - r->fh.padding, r->buf2, BSIZE, x, y, w, h,0,0);
                    result->buffer = r->buf2;
                }
                else {
                    result->buffer = r->buf;
                    result->size = r->fh.size - r->fh.padding;
                }
                result->data_type = OBR_DT_JPEG;
                result->width = w;
                result->height = h;
            }
            else {
                result->data_type = OBR_DT_NONE;
            }
        }
        else {
            result->data_type = OBR_DT_NONE;
        }
    }

    return 0;
}

int obr_snapshot(int ch, time_t ts, void *buf, int bsize, int x, int y, int w, int h)
{
	char				log_msg[256];
	int					ret;
    void *handle = obr_alloc(ch, 0, ts-10, ts);
    if(handle == 0)
        return -1;
    obr_result_t result;
    obr_setup(handle , OBR_EV_VIDEO, OBR_IMG_NONE, OBR_ORDER_DESCENT);
    if((ret = obr_read(handle , &result)) < 0) {
        //ob_error("snapshot read failed\n");
		sprintf(log_msg,"[RD] obr_read ret = %d\n",ret);
		Write_Log(0,log_msg);
		sprintf(log_msg,"[RD] snapshot read failed\n");
		Write_Log(0,log_msg);
        return -2;
    }

    if(w == 0 || h == 0) {
        x = 0;
        y = 0;
        w = result.width;
        h = result.height;
    }
    else {
        if(w > result.width)
            w = result.width;
        if(x > result.width || (x + w) > result.width)
            x = result.width - w;
        if(h > result.height)
            h = result.height;
        if(y > result.height || (y + h) > result.height)
            y = result.height - h;
    }

    int psize = 0;
    if(result.data_type == OBR_DT_H264) {
        psize = h264tojpeg(result.buffer, result.size, buf, bsize, x, y, w, h,0,0);
        //ob_debug("#%d snapshot : %dx%d, %dx%d @ %u.%06u (%d)\n", ch, x, y, w, h, (u32)result.sec, (u32)result.usec, psize);
		sprintf(log_msg,"[RD] #%d snapshot : %dx%d, %dx%d @ %u.%06u (%d)\n", ch, x, y, w, h, (u32)result.sec, (u32)result.usec, psize);
		Write_Log(0,log_msg);
	}
    else {
        //ob_debug("#%d snapshot : %dx%d @ %u.%06u (%d)\n", ch, result.width, result.height, (u32)result.sec, (u32)result.usec, result.size);
        sprintf(log_msg,"[RD] #%d snapshot : %dx%d @ %u.%06u (%d)\n", ch, result.width, result.height, (u32)result.sec, (u32)result.usec, result.size);
		Write_Log(0,log_msg);
		if(result.size < bsize) {
            memcpy(buf, result.buffer, result.size);
            psize = result.size;
        }
    }

    obr_free(handle);

    return psize;
}

