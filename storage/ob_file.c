/*!
 *  @file       ob_file.c
 *  @version    1.0
 *  @date       04/08/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       Orbit File
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "ob_platform.h"
#include "ob_file.h"
#include "dbg_msg/dbg_log.h"


// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "OBF"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------

/*
 * 1. ob_if = ob_vf (only headers & index)
 * 2. filename determined by recorder & reader
 * 3. duration assigned by recorder, or read in file header
 * 4. open for read or write
 * 5. own frame header with delta time
 * 6.
 *
 */

typedef struct __attribute__ ((packed)) {

    u32     magic;
    u32     version;
    u32     start_time; // in seconds
    u32     duration;   // in seconds
    u32     flags;
    u32     data_offset;

    u32     resv[122];

} ob_ifile_t;

#define OBIF_MAGIC      0x6f426946      // 'oBiF'
#define OBIF_VERSION    0x20140403

#define OBFF_SZ_WRAP(X)     (((X)+7) & ~7)

#define OBV_MAX_DURATION    86400

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------

#define OBFC_MAGIC   0x6f42664d      // 'oBfM'
typedef struct {

    u32     magic;
    int     readonly;
    char    vfilename[132];
    char    ifilename[132];
    u32     start_time;
    u32     duration;
    int     vfd;
    int     ifd;
    int     is_backward;

    ob_ifile_t  header;
    u64         *offsets;

    u32     last_index;
    u64     offset;

    char    padding[256];

} obf_ctrl_t;

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------

//static char				log_msg[256];

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
static int verify_header(ob_ifile_t *ifh, const char *filename)
{
	char				log_msg[256];
    if(ifh->magic != OBIF_MAGIC) {
        //ob_error("%s wrong file magic\n", filename);
		sprintf(log_msg,"[OBF] %s wrong file magic\n", filename);
		Write_Log(0,log_msg);
        return -1;
    }
    if(ifh->version != OBIF_VERSION) {
        //ob_error("%s wrong file version: %08x\n", filename, ifh->version);
		sprintf(log_msg,"[OBF] %s wrong file version: %08x\n", filename, ifh->version);
		Write_Log(0,log_msg);
        return -1;
    }
    if(ifh->duration > OBV_MAX_DURATION) {
        //ob_error("%s duration is too long : %u\n", filename, ifh->duration);
		sprintf(log_msg,"[OBF] %s duration is too long : %u\n", filename, ifh->duration);
		Write_Log(0,log_msg);
        return -1;
    }
    if(ifh->data_offset != sizeof(ob_ifile_t)) {
        //ob_error("%s wrong data offset : 0x%08x\n", filename, ifh->data_offset);
		sprintf(log_msg,"[OBF] %s wrong data offset : 0x%08x\n", filename, ifh->data_offset);
		Write_Log(0,log_msg);
        return -1;
    }
    return 0;
}

static int fetch_header(ob_ifile_t *ifh, u64 **offsets, const char *filename)
{
	char				log_msg[256];
    int status = -1;
    int fd = open(filename, O_RDONLY | O_LARGEFILE);
    if(fd >= 0) {
        u32 hsize = sizeof(ob_ifile_t);
        u32 isize = 0;
        int ret = ob_read(fd, 0ULL, ifh, hsize);
        if(ret == hsize) {
            if(verify_header(ifh, filename) == 0) {
                if(offsets) {
                    isize = ifh->duration * 8;
                    *offsets = (u64 *)malloc(isize);
                    if(*offsets) {
                        memset(*offsets, 0, isize);
                        ret = ob_read(fd, ifh->data_offset, *offsets, isize);
                        if(ret > 0) {
                            status = 0;
                        }
                        else {
                            //ob_error("%s read index failed\n", filename);
							sprintf(log_msg,"[OBF] %s read index failed\n", filename);
							Write_Log(0,log_msg);
                            free(*offsets);
                            *offsets = 0;
                        }
                    }
                    else {
                        //ob_error("%s OOM for index buffer\n", filename);
						sprintf(log_msg,"[OBF] %s OOM for index buffer\n", filename);
						Write_Log(0,log_msg);
                        close(fd);
                        return -1;
                    }
                }
                else
                    status = 0;
            }
            else {
                //ob_error("%s verify header failed\n", filename);
				sprintf(log_msg,"[OBF] %s verify header failed\n", filename);
				Write_Log(0,log_msg);
            }
        }
        else {
            //ob_error("%s read header failed\n", filename);
			sprintf(log_msg,"[OBF] %s read header failed\n", filename);
			Write_Log(0,log_msg);
        }
        close(fd);
    }
    else {
        //ob_error("%s open failed\n", filename);
		sprintf(log_msg,"[OBF] %s open failed\n", filename);
		Write_Log(0,log_msg);
    }

    return status;
}

static int update_offsets(obf_ctrl_t *f, u32 index, u32 number)
{
	char				log_msg[256];
    if(f->readonly || f->ifd < 0 || f->offsets == 0)
        return 0;
    if(index >= f->duration)
        return 0;
    u32 s_off = f->header.data_offset + index * 8;
    u32 size = number * 8;
    int ret = ob_write(f->ifd, s_off, f->offsets+index, size);
    if(ret != size) {
       // ob_error("%s write index failed [%u,%u]\n", f->ifilename, index, number);
		sprintf(log_msg,"[OBF] %s write index failed [%u,%u]\n", f->ifilename, index, number);
		Write_Log(0,log_msg);
        return -1;
    }
    return 0;
}

static int open_for_read(obf_ctrl_t *f)
{
	char				log_msg[256];
    f->vfd = open(f->vfilename, O_RDONLY | O_LARGEFILE);
    if(f->vfd < 0) {
        //ob_error("%s open failed\n", f->vfilename);
		sprintf(log_msg,"[OBF] %s open failed\n", f->vfilename);
		Write_Log(0,log_msg);
        return -1;
    }

    if(fetch_header(&f->header, &f->offsets, f->ifilename) < 0) {
        //ob_error("%s read header failed\n", f->ifilename);
		sprintf(log_msg,"[OBF] %s read header failed\n", f->ifilename);
		Write_Log(0,log_msg);
        return -1;
    }

    f->start_time = f->header.start_time;
    f->duration = f->header.duration;
    f->offset = sizeof(ob_ifile_t);

    return 0;
}

static int open_for_write(obf_ctrl_t *f)
{
	char				log_msg[256];
    if(fetch_header(&f->header, &f->offsets, f->ifilename) == 0) {
        if(f->header.start_time != f->start_time || f->header.duration != f->duration) {
            free(f->offsets);
            f->offsets = 0;
            //ob_error("%s header mismatch\n", f->ifilename);
			sprintf(log_msg,"[OBF] %s header mismatch\n", f->ifilename);
			Write_Log(0,log_msg);
        }
        else {
            f->ifd = open(f->ifilename, O_WRONLY | O_LARGEFILE | O_SYNC);
            f->vfd = open(f->vfilename, O_WRONLY | O_LARGEFILE | O_SYNC);
            if(f->vfd < 0) {
                close(f->ifd);
                f->ifd = -1;
            }
            else {
                f->offset = (u64)lseek64(f->vfd, 0, SEEK_END);
                //ob_debug("%s Continue @ 0x%qx\n", f->vfilename, f->offset);
				sprintf(log_msg,"[OBF] %s Continue @ 0x%qx\n", f->vfilename, f->offset);
				Write_Log(0,log_msg);
                if(f->offset > sizeof(ob_ifile_t))
                    return 0;
                else {
                    close(f->ifd);
                    f->ifd = -1;
                }
            }
        }
    }

    // New file
    f->ifd = open(f->ifilename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE | O_SYNC);
    if(f->ifd < 0) {
        //ob_error("%s create failed\n", f->ifilename);
		sprintf(log_msg,"[OBF] %s create failed\n", f->ifilename);
		Write_Log(0,log_msg);
        return -1;
    }
    f->vfd = open(f->vfilename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE | O_SYNC);
    if(f->vfd < 0) {
        //ob_error("%s create failed\n", f->vfilename);
		sprintf(log_msg,"[OBF] %s create failed\n", f->vfilename);
		Write_Log(0,log_msg);
        return -1;
    }

    u32 hsize = sizeof(ob_ifile_t);
    u32 isize = f->duration * 8;

    if(f->offsets == 0) {
        f->offsets = (u64 *)malloc(isize);
        if(f->offsets == 0) {
            //ob_error("%s OOM for index allocation\n", f->ifilename);
			sprintf(log_msg,"[OBF] %s OOM for index allocation\n", f->ifilename);
			Write_Log(0,log_msg);
            return -1;
        }
    }
    memset(f->offsets, 0, isize);
    f->header.magic = OBIF_MAGIC;
    f->header.version = OBIF_VERSION;
    f->header.start_time = f->start_time;
    f->header.duration = f->duration;
    f->header.data_offset = hsize;

    int ret = ob_write(f->ifd, 0ULL, &f->header, hsize);
    if(ret != hsize) {
        //ob_error("%s write header failed\n", f->ifilename);
		sprintf(log_msg,"[OBF] %s write header failed\n", f->ifilename);
		Write_Log(0,log_msg);
        return -1;
    }
    ret = ob_write(f->ifd, hsize, f->offsets, isize);
    if(ret != isize) {
        //ob_error("%s write index failed\n", f->ifilename);
		sprintf(log_msg,"[OBF] %s write index failed\n", f->ifilename);
		Write_Log(0,log_msg);
        return -1;
    }
    ret = ob_write(f->vfd, 0ULL, &f->header, hsize);
    if(ret != hsize) {
        //ob_error("%s write header failed\n", f->vfilename);
		sprintf(log_msg,"[OBF] %s write header failed\n", f->vfilename);
		Write_Log(0,log_msg);
        return -1;
    }

    f->offset = hsize;

    return 0;
}

static int next_key(obf_ctrl_t *f, u32 ts)
{
	char				log_msg[256];
    assert(f->offsets);

    //ob_debug("seek to %u [%u-%u]\n", ts, f->start_time, f->start_time + f->duration-1);

    f->offset = 0;
    if(ts && ts >= f->start_time) {
        u32 idx = ts - f->start_time;
        if(idx < f->duration) {

            //ob_debug("[%u] %qu\n", idx, f->offsets[idx]);

            if(f->offsets[idx] && f->offsets[idx] != f->offset) {
                f->offset = f->offsets[idx];
                return 0;
            }
            else {
                while(++idx < f->duration) {
                    if(f->offsets[idx] && f->offsets[idx] != f->offset) {
                        f->offset = f->offsets[idx];
                        return 0;
                    }
                }
            }
        }
        else
            return -1;
    }
    return -1;
}

static int prev_key(obf_ctrl_t *f, u32 ts)
{
	char				log_msg[256];
    assert(f->offsets);

    f->offset = 0;
    if(ts && ts >= f->start_time) {
        u32 idx = ts - f->start_time;
        if(idx < f->duration) {
            if(f->offsets[idx] && f->offsets[idx] != f->offset) {
                f->offset = f->offsets[idx];
                return 0;
            }
            else {
                while(idx) {
                    idx--;
                    if(f->offsets[idx] && f->offsets[idx] != f->offset) {
                        f->offset = f->offsets[idx];
                        return 0;
                    }
                }
            }
        }
        else
            return -1;
    }
    return -1;
}

static int find_key(obf_ctrl_t *f, u32 ts)
{
    if(f->is_backward)
        return prev_key(f, ts-1);
    else
        return next_key(f, ts+1);
}

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------
int obf_close(void *handle)
{
    obf_ctrl_t *f = (obf_ctrl_t *)handle;
    if(f && f->magic == OBFC_MAGIC) {
        if(f->ifd >= 0) {
            close(f->ifd);
            f->ifd = -1;
        }
        if(f->vfd >= 0) {
            //sync_voffsets(f);
            close(f->vfd);
            f->vfd = -1;
        }
        if(f->offsets) {
            free(f->offsets);
            f->offsets = 0;
        }
    }
    free(f);
    return 0;
}

void *obf_open(const char *filename, int readonly, u32 start, u32 duration)
{
	char				log_msg[256];
    assert(filename);
    assert(duration <= OBV_MAX_DURATION);

    obf_ctrl_t *f = (obf_ctrl_t *)malloc(sizeof(obf_ctrl_t));
    if(!f) {
        //ob_error("OOM on obf alloc\n");
		sprintf(log_msg,"[OBF] OOM on obf alloc\n");
		Write_Log(0,log_msg);
        return 0;
    }
    memset(f, 0, sizeof(obf_ctrl_t));
    f->vfd = -1;
    f->ifd = -1;
    f->readonly = readonly;
    strncpy(f->vfilename, filename, 128);
    sprintf(f->ifilename, "%s.idx", filename);
    f->magic = OBFC_MAGIC;

    if(readonly) {
        if(open_for_read(f) < 0) {
            obf_close(f);
            return 0;
        }
    }
    else {
        f->start_time = start;
        f->duration = duration;
        if(open_for_write(f) < 0) {
            obf_close(f);
            return 0;
        }
    }

    return f;
}

int obf_write(void *handle, as_frame_t *fh, u8 *data)
{
	char				log_msg[256];
    obf_ctrl_t *f = (obf_ctrl_t *)handle;
    if(!f || f->magic != OBFC_MAGIC || f->readonly)
        return -1;

    u32 size = OBFF_SZ_WRAP(fh->size);
    fh->padding = size - fh->size; // what if the given padding is not 0
    fh->size = size;

    u32 idx = fh->sec - f->start_time;
    if(idx >= f->duration) {
        if((fh->flags & ASF_FLAG_KEY) || idx > f->duration) {
            //ob_debug("%s full (%u = %u - %u > %u)\n", f->vfilename, idx, fh->sec, f->start_time, f->duration);
            sprintf(log_msg,"[OBF] %s full (%u = %u - %u > %u)\n", f->vfilename, idx, fh->sec, f->start_time, f->duration);
			Write_Log(0,log_msg);
			return 0;
        }
    }

    int iovcnt = 0;
    struct iovec iov[3];
    iov[0].iov_base = fh;
    iov[0].iov_len = sizeof(as_frame_t);
    iov[1].iov_base = data;
    iov[1].iov_len = fh->size - fh->padding;
    if(fh->padding) {
        iov[2].iov_base = f->padding;
        iov[2].iov_len = fh->padding;
        iovcnt = 3;
    }
    else
        iovcnt = 2;

    int ssize = size + sizeof(as_frame_t);
    int ret = ob_writev(f->vfd, f->offset, iov, iovcnt);
    if(ret != ssize) {
        //ob_error("%s write frame failed = %d (%d)\n", f->vfilename, ret, ssize);
		sprintf(log_msg,"[OBF] %s write frame failed = %d (%d)\n", f->vfilename, ret, ssize);
		Write_Log(0,log_msg);
        return -1;
    }

    if(fh->flags & ASF_FLAG_KEY) {
        if(idx != f->last_index) {
            if(f->offsets[idx]) {
                //ob_error("%s index write twice @ %u\n", f->vfilename, idx);
				sprintf(log_msg,"[OBF] %s index write twice @ %u\n", f->vfilename, idx);
				Write_Log(0,log_msg);
            }
            f->offsets[idx] = f->offset;
            if(update_offsets(f, idx, 1) < 0) {
                //ob_error("%s write index failed\n", f->vfilename);
				sprintf(log_msg,"[OBF] %s write index failed\n", f->vfilename);
				Write_Log(0,log_msg);
            }
            f->last_index = idx;
        }
    }

    f->offset += ssize;

    return ret;
}

int obf_seek(void *handle, u32 ts)
{
    obf_ctrl_t *f = (obf_ctrl_t *)handle;
    if(!f || f->magic != OBFC_MAGIC || !f->readonly)
        return -1;

    if(f->is_backward)
        return prev_key(f, ts);
    else
        return next_key(f, ts);
}

int obf_read(void *handle, as_frame_t *fh, u8 *data, int bsize)
{
	char				log_msg[256];
    obf_ctrl_t *f = (obf_ctrl_t *)handle;
    if(!f || f->magic != OBFC_MAGIC || !f->readonly)
        return -1;

    do {
        if(f->offset == 0)
            return -1;

        int ret = ob_read(f->vfd, f->offset, fh, sizeof(as_frame_t));
        if(ret != sizeof(as_frame_t))
            return -1;
        if(fh->magic != ASF_MAGIC) {
            //ob_error("%s wrong frame magic @ 0x%qx\n", f->vfilename, f->offset);
			sprintf(log_msg,"[OBF] %s wrong frame magic @ 0x%qx\n", f->vfilename, f->offset);
			Write_Log(0,log_msg);
            if(find_key(f, fh->sec) < 0)
                return -1;
            continue;
        }

        if(fh->size > bsize) {
            //ob_error("%s oversized frame @ 0x%qx\n", f->vfilename, f->offset);
			sprintf(log_msg,"[OBF] oversized frame @ 0x%qx\n", f->vfilename, f->offset);
			Write_Log(0,log_msg);
            if(find_key(f, fh->sec) < 0)
                return -1;
            continue;
        }

        f->offset += sizeof(as_frame_t);
        ret = ob_read(f->vfd, f->offset, data, fh->size);
        if(ret != fh->size) {
            //ob_error("%s read frame error @ 0x%qx\n", f->vfilename, f->offset);
			sprintf(log_msg,"[OBF] %s read frame error @ 0x%qx\n", f->vfilename, f->offset);
			Write_Log(0,log_msg);
            return -1;
        }

        if(f->is_backward) {
            if(prev_key(f, fh->sec-1) < 0)
                f->offset = 0;
        }
        else
            f->offset += fh->size;
        break;

    } while(1);

    return 0;
}

int obf_set_dir(void *handle, int is_backward)
{
    obf_ctrl_t *f = (obf_ctrl_t *)handle;
    if(!f || f->magic != OBFC_MAGIC || !f->readonly)
        return -1;
    f->is_backward = is_backward;
    return 0;
}

int obf_info(void *handle, u32 *start, u32 *end)
{
    obf_ctrl_t *f = (obf_ctrl_t *)handle;
    if(!f || f->magic != OBFC_MAGIC || !f->readonly)
        return -1;

    if(start)
        *start = f->start_time;
    if(end)
        *end = f->start_time + f->duration - 1;

    return 0;
}

