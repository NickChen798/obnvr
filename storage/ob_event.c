/*!
 *  @file       ob_event.c
 *  @version    1.0
 *  @date       07/08/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       Event File
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "ob_platform.h"
#include "ob_event.h"
#include "dbg_msg/dbg_log.h"


// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "OBE"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------

#define OBEF_MAGIC      0x6f426546      // 'oBeF'
#define OBEF_VERSION    0x20140403

typedef struct __attribute__ ((packed)) {

    u32     magic;
    u32     version;
    u32     start_time; // in seconds
    u32     duration;   // in seconds
    u32     flags;
    u32     data_offset;

    u32     resv[122];

} ob_efile_t;

typedef struct __attribute__ ((packed)) {

    u8      ch;
    u8      id;
    u8      type;
    u8      flags;
    u32     sec;
    u32     usec;
    u32     data[5];

} obe_entry_t;

typedef struct __attribute__ ((packed)) {

    u16  x;
    u16  y;
    u16  width;
    u16  height;
    char plate[OBE_PLATE_LEN];

} obe_plate_t;

#define OBV_MAX_DURATION    86400

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------

#define OBEC_MAGIC  0x6f42654d      // 'oBeM'
typedef struct {

    u32     magic;
    int     readonly;
    char    filename[132];
    u32     start_time;
    u32     duration;
    int     fd;
    int     is_backward;

    ob_efile_t  header;
    obe_entry_t entry;

    u64     offset;

    int     rindex;
    u32     rcount;
    u32     rsize;
    void    *rbuf;

} obe_ctrl_t;

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------

//static char				log_msg[256];

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
static int verify_header(ob_efile_t *efh, const char *filename)
{
	char				log_msg[256];
    if(efh->magic != OBEF_MAGIC) {
        //ob_error("%s wrong file magic\n", filename);
		sprintf(log_msg,"[OBE] %s wrong file magic\n", filename);
		Write_Log(0,log_msg);
        return -1;
    }
    if(efh->version != OBEF_VERSION) {
        //ob_error("%s wrong file version: %08x\n", filename, efh->version);
		sprintf(log_msg,"[OBE] wrong file version: %08x\n", filename, efh->version);
		Write_Log(0,log_msg);
        return -1;
    }
    if(efh->duration > OBV_MAX_DURATION) {
        //ob_error("%s duration is too long : %u\n", filename, efh->duration);
		sprintf(log_msg,"[OBE] %s duration is too long : %u\n", filename, efh->duration);
		Write_Log(0,log_msg);
        return -1;
    }
    if(efh->data_offset != sizeof(ob_efile_t)) {
        //ob_error("%s wrong data offset : 0x%08x\n", filename, efh->data_offset);
		sprintf(log_msg,"[OBE] %s wrong data offset : 0x%08x\n", filename, efh->data_offset);
		Write_Log(0,log_msg);
        return -1;
    }
    return 0;
}

static int fetch_header(ob_efile_t *efh, const char *filename)
{
	char				log_msg[256];
    int status = -1;
    int fd = open(filename, O_RDONLY | O_LARGEFILE);
    if(fd >= 0) {
        u32 hsize = sizeof(ob_efile_t);
        int ret = ob_read(fd, 0ULL, efh, hsize);
        if(ret == hsize) {
            if(verify_header(efh, filename) == 0) {
                status = 0;
            }
            else {
                //ob_error("%s verify header failed\n", filename);
				sprintf(log_msg,"[OBE] %s verify header failed\n", filename);
				Write_Log(0,log_msg);
            }
        }
        else {
            //ob_error("%s read header failed\n", filename);
			sprintf(log_msg,"[OBE] %s read header failed\n", filename);
			Write_Log(0,log_msg);
        }
        close(fd);
    }
    else {
        //ob_error("%s open failed\n", filename);
		sprintf(log_msg,"[OBE] %s open failed\n", filename);
		Write_Log(0,log_msg);
    }

    return status;
}

static int open_for_read(obe_ctrl_t *f)
{
	char				log_msg[256];
    f->fd = open(f->filename, O_RDONLY | O_LARGEFILE);
    if(f->fd < 0) {
		sprintf(log_msg,"[OBE] %s open failed (%s)\n", f->filename, strerror(errno));
		Write_Log(0,log_msg);
        //ob_error("%s open failed (%s)\n", f->filename, strerror(errno));
        return -1;
    }

    if(fetch_header(&f->header, f->filename) < 0) {
        //ob_error("%s read header failed\n", f->filename);
		sprintf(log_msg,"[OBE] %s read header failed\n", f->filename);
		Write_Log(0,log_msg);
        return -1;
    }

    f->start_time = f->header.start_time;
    f->duration = f->header.duration;
    f->offset = f->header.data_offset;
    f->rcount = 0;
    f->rindex = -1;

    f->rsize = (u32)(lseek64(f->fd, 0, SEEK_END) - f->offset);
    if(f->rsize > 0x800000) {
        //ob_error("%s file too large (%u)\n", f->filename, f->rsize);
		sprintf(log_msg,"[OBE] %s file too large (%u)\n", f->filename, f->rsize);
		Write_Log(0,log_msg);
        f->rsize = 0;
        return -1;
    }
    f->rbuf = malloc(f->rsize);
    if(f->rbuf == 0) {
        //ob_error("%s oom\n", f->filename);
		sprintf(log_msg,"[OBE] %s oom\n", f->filename);
		Write_Log(0,log_msg);
        return -1;
    }
    int rsize = ob_read(f->fd, f->offset, f->rbuf, f->rsize);
    if(rsize <= 0) {
        //ob_error("%s read failed\n", f->filename);
		sprintf(log_msg,"[OBE] %s read failed\n", f->filename);
		Write_Log(0,log_msg);
        return -1;
    }
    if(rsize < f->rsize) {
        //ob_error("%s read %d < %u\n", f->filename, rsize, f->rsize);
		sprintf(log_msg,"[OBE] %s read %d < %u\n", f->filename, rsize, f->rsize);
		Write_Log(0,log_msg);
        f->rsize = rsize;
    }
    close(f->fd);
    f->fd = -1;

    f->rcount = f->rsize / sizeof(obe_entry_t);
    //ob_debug("%s: read %u events, %u bytes\n", f->filename, f->rcount, f->rsize);
	sprintf(log_msg,"[OBE] %s: read %u events, %u bytes\n", f->filename, f->rcount, f->rsize);
	Write_Log(0,log_msg);

    return 0;
}

static int open_for_write(obe_ctrl_t *f)
{
	char				log_msg[256];
    if(fetch_header(&f->header, f->filename) == 0) {
        if(f->header.start_time != f->start_time || f->header.duration != f->duration) {
            //ob_error("%s header mismatch\n", f->filename);
			sprintf(log_msg,"[OBE] %s header mismatch\n", f->filename);
			Write_Log(0,log_msg);
        }
        else {
            f->fd = open(f->filename, O_WRONLY | O_LARGEFILE | O_SYNC);
            if(f->fd >= 0) {
                f->offset = (u64)lseek64(f->fd, 0, SEEK_END);
                u64 doff = f->offset - f->header.data_offset;
                if((doff % sizeof(obe_entry_t)) == 0) {
                    //ob_debug("%s Continue @ 0x%qx\n", f->filename, f->offset);
					sprintf(log_msg,"[OBE] %s Continue @ 0x%qx\n", f->filename, f->offset);
					Write_Log(0,log_msg);
                    return 0;
                }
                else {
                    doff = (doff / sizeof(obe_entry_t)) * sizeof(obe_entry_t);
                    doff += f->header.data_offset;
                    //ob_error("%s start offset shift : 0x%qx->0x%qx\n", f->filename, f->offset, doff);
					sprintf(log_msg,"[OBE] %s start offset shift : 0x%qx->0x%qx\n", f->filename, f->offset, doff);
					Write_Log(0,log_msg);
                    f->offset = (u64)lseek64(f->fd, doff, SEEK_SET);
                    if(f->offset == doff) {
                        //ob_debug("%s Continue @ 0x%qx\n", f->filename, f->offset);
						sprintf(log_msg,"[OBE] %s Continue @ 0x%qx\n", f->filename, f->offset);
						Write_Log(0,log_msg);
                        return 0;
                    }
                    //ob_error("%s seek to 0x%qx failed\n", f->filename, doff);
					sprintf(log_msg,"[OBE] %s seek to 0x%qx failed\n", f->filename, doff);
					Write_Log(0,log_msg);
                }
                close(f->fd);
                f->fd = -1;
            }
        }
    }

    // New file
    f->fd = open(f->filename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE | O_SYNC);
    if(f->fd < 0) {
        //ob_error("%s create failed\n", f->filename);
		sprintf(log_msg,"[OBE] %s create failed\n", f->filename);
		Write_Log(0,log_msg);
        return -1;
    }

    u32 hsize = sizeof(ob_efile_t);

    f->header.magic = OBEF_MAGIC;
    f->header.version = OBEF_VERSION;
    f->header.start_time = f->start_time;
    f->header.duration = f->duration;
    f->header.data_offset = hsize;

    int ret = ob_write(f->fd, 0ULL, &f->header, hsize);
    if(ret != hsize) {
        //ob_error("%s write header failed\n", f->filename);
		sprintf(log_msg,"[OBE] %s write header failed\n", f->filename);
		Write_Log(0,log_msg);
        return -1;
    }

    f->offset = hsize;

    return 0;
}

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------
int obe_close(void *handle)
{
    obe_ctrl_t *f = (obe_ctrl_t *)handle;
    if(f && f->magic == OBEC_MAGIC) {
        if(f->fd >= 0) {
            close(f->fd);
            f->fd = -1;
        }
        if(f->rbuf) {
            free(f->rbuf);
            f->rbuf = 0;
        }
    }
    free(f);
    return 0;
}

void *obe_open(const char *filename, int readonly, u32 start, u32 duration)
{
	char				log_msg[256];
    assert(filename);
    assert(duration <= OBV_MAX_DURATION);

    obe_ctrl_t *f = (obe_ctrl_t *)malloc(sizeof(obe_ctrl_t));
    if(!f) {
        //ob_error("OOM on obf alloc\n");
		sprintf(log_msg,"[OBE] OOM on obf alloc\n");
		Write_Log(0,log_msg);
        return 0;
    }
    memset(f, 0, sizeof(obe_ctrl_t));
    f->fd = -1;
    f->readonly = readonly;
    strncpy(f->filename, filename, 128);
    f->magic = OBEC_MAGIC;

    if(readonly) {
        if(open_for_read(f) < 0) {
            obe_close(f);
            return 0;
        }
    }
    else {
        f->start_time = start;
        f->duration = duration;
        if(open_for_write(f) < 0) {
            obe_close(f);
            return 0;
        }
    }

    return f;
}

int obe_write(void *handle, ob_event_t *ev)
{
	char				log_msg[256];
    obe_ctrl_t *f = (obe_ctrl_t *)handle;
    if(!f || f->magic != OBEC_MAGIC || f->readonly)
        return -1;

    if(ev->sec >= (f->start_time + f->duration))
        return 0;

    obe_entry_t *e = &f->entry;
    e->ch = ev->ch;
    e->id = ev->id;
    e->type = ev->type;
    e->flags = ev->flags;
    e->sec = ev->sec;
    e->usec = ev->usec;
    if(ev->type == OBE_TYPE_PLATE) {
        obe_plate_t *ep = (obe_plate_t *)e->data;
        ep->x = ev->x;
        ep->y = ev->y;
        ep->width = ev->width;
        ep->height = ev->height;
        memcpy(ep->plate, ev->plate, OBE_PLATE_LEN);
    }
    else
        memset(e->data, 0, sizeof(e->data));

    int wsize = sizeof(obe_entry_t);
    int ret = ob_write(f->fd, f->offset, e, wsize);
    if(ret != wsize) {
        //ob_error("%s write failed = %d (%d)\n", f->filename, ret, wsize);
		sprintf(log_msg,"[OBE] %s write failed = %d (%d)\n", f->filename, ret, wsize);
		Write_Log(0,log_msg);
        return -1;
    }

    f->offset += wsize;

    return ret;
}

int obe_seek(void *handle, u32 ts)
{
    obe_ctrl_t *f = (obe_ctrl_t *)handle;
    if(!f || f->magic != OBEC_MAGIC || !f->readonly)
        return -1;

    if(f->rsize == 0 || f->rbuf == 0)
        return -1;

    int i;
    obe_entry_t *e = (obe_entry_t *)f->rbuf;
    if(f->is_backward) {
        for(i=f->rcount-1;i>0;i--) {
            if(e[i].sec <= ts)
                break;
        }
        if(i>0) {
            f->rindex = i;
            return 0;
        }
    }
    else {
        for(i=0;i<f->rcount;i++) {
            if(e[i].sec >= ts)
                break;
        }
        if(i<f->rcount) {
            f->rindex = i;
            return 0;
        }
    }
    f->rindex = -1;
    return -1;
}

int obe_read(void *handle, u8 ch, u8 id, u8 type, ob_event_t *ev)
{
    obe_ctrl_t *f = (obe_ctrl_t *)handle;
    if(!f || f->magic != OBEC_MAGIC || !f->readonly)
        return -1;

    if(f->rsize == 0 || f->rbuf == 0)
        return -1;

    int valid = 0;
    while(f->rindex >= 0 && f->rindex < f->rcount && !valid) {

        obe_entry_t *e = (obe_entry_t *)f->rbuf + f->rindex;
        if((type == OBE_TYPE_ALL || type == e->type) &&
           (ch == OBE_CH_ALL || (ch == e->ch && id == e->id))) {

            ev->ch = e->ch;
            ev->id = e->id;
            ev->type = e->type;
            ev->flags = e->flags;
            ev->sec = e->sec;
            ev->usec = e->usec;
            if(e->type == OBE_TYPE_PLATE) {
                obe_plate_t *ep = (obe_plate_t *)e->data;
                ev->x = ep->x;
                ev->y = ep->y;
                ev->width = ep->width;
                ev->height = ep->height;
                memcpy(ev->plate, ep->plate, OBE_PLATE_LEN);
            }
            valid = 1;
        }

        if(f->is_backward)
            f->rindex --;
        else
            f->rindex ++;
    }

    return (valid) ? 0 : -1;
}

int obe_set_dir(void *handle, int is_backward)
{
    obe_ctrl_t *f = (obe_ctrl_t *)handle;
    if(!f || f->magic != OBEC_MAGIC || !f->readonly)
        return -1;
    f->is_backward = is_backward;
    return 0;
}

int obe_info(void *handle, u32 *start, u32 *end)
{
    obe_ctrl_t *f = (obe_ctrl_t *)handle;
    if(!f || f->magic != OBEC_MAGIC || !f->readonly)
        return -1;

    if(start)
        *start = f->start_time;
    if(end)
        *end = f->start_time + f->duration - 1;

    return 0;
}

