#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <poll.h>
#include <malloc.h>
#include <netdb.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/statfs.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/if.h>
#include <linux/cdrom.h>
#include <linux/types.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <dirent.h>

//#define NDEBUG
#include <assert.h>

#ifndef __OB_PLATFORM_H__
#define __OB_PLATFORM_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------
#define u8      uint8_t
#define u16     uint16_t
#define u32     uint32_t
#define u64     uint64_t
#define i8      int8_t
#define i16     int16_t
#define i32     int32_t
#define i64     int64_t

// ----------------------------------------------------------------------------
// Macro Definition
// ----------------------------------------------------------------------------
#define dbg         printf
#define gettid()	syscall(__NR_gettid)

// ----------------------------------------------------------------------------
// inline functions
// ----------------------------------------------------------------------------
static inline void msleep(int msec)
{
    struct timespec ms_delay;
    ms_delay.tv_sec = ((msec)>=1000) ? (msec)/1000 : 0;
    ms_delay.tv_nsec = ((msec)%1000) * 1000000;
    nanosleep(&ms_delay, NULL);
}

static inline u64 ob_msec(void)
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL) != 0)
		return 0;
	return (((u64)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

static inline u64 ob_usec(void)
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL) != 0)
		return 0;
	return (((u64)tv.tv_sec) * 1000000) + tv.tv_usec;
}

/*!
 * @brief   Create normal thread
 * @param[in]   thrd        pointer to thread entry
 * @param[in]   arg         pointer to argument
 * @return  0 on success, otherwise < 0
 */
static inline int create_thread(void* thrd, void *arg)
{
    pthread_t thread_id;

    if(pthread_create(&thread_id, NULL, thrd, arg) != 0) {
        dbg("normal thread create error: %s\n", strerror(errno));
        return -1;
    }

    pthread_detach(thread_id);

    return 0;
}

// ----------------------------------------------------------------------------
// Disk APIs
// ----------------------------------------------------------------------------
static inline int ob_read(int f, u64 off, void *p, u32 size)
{
    if(lseek64(f, off, SEEK_SET) != off) {
        dbg("seek to %qu error: %s\n", off, strerror(errno));
        return -1;
    }

    int rsize;
    if((rsize = read(f, p, size)) < 0) {
        dbg("read %u bytes to %p from %qu error:%s\n",
                                    size, p, off, strerror(errno));
        return -1;
    }
    return rsize;
}

static inline int ob_write(int f, u64 off, void *p, u32 size)
{
    if(lseek64(f, off, SEEK_SET) != off) {
        dbg("seek to %qu error: %s\n", off, strerror(errno));
        return -1;
    }

    int wsize;
    if((wsize = write(f, p, size)) < 0) {
        dbg("write %u bytes to %qu from %p error:%s\n",
                                    size, off, p, strerror(errno));
        return -1;
    }

    return wsize;
}

static inline int ob_readv(int f, u64 off, const struct iovec *iov, int iovcnt)
{
    if(lseek64(f, off, SEEK_SET) != off) {
        dbg("seek to %qu error: %s\n", off, strerror(errno));
        return -1;
    }

    int rsize;
    if((rsize = readv(f, iov, iovcnt)) < 0) {
        dbg("readv to from %qu error:%s\n", off, strerror(errno));
        return -1;
    }
    return rsize;
}

static inline int ob_writev(int f, u64 off, const struct iovec *iov, int iovcnt)
{
    if(lseek64(f, off, SEEK_SET) != off) {
        dbg("seek to %qu error: %s\n", off, strerror(errno));
        return -1;
    }

    int wsize;
    if((wsize = writev(f, iov, iovcnt)) < 0) {
        dbg("writev from to %qu error:%s\n", off, strerror(errno));
        return -1;
    }
    return wsize;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OB_PLATFORM_H__

