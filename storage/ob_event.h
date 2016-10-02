/*!
 *  @file       ob_event.h
 *  @version    1.0
 *  @date       07/08/2013
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       Event File
 */

#ifndef __OB_EVENT_H__
#define __OB_EVENT_H__

#include "ob_platform.h"

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------
#define OBE_PLATE_LEN       12
#define OBE_TYPE_PLATE      1
#define OBE_TYPE_VLOSS      2
#define OBE_TYPE_VRECOVERY  3

#define OBE_TYPE_ALL        0xff
#define OBE_CH_ALL          0xff

#define OBE_FLAG_CARIN      (1 << 0)

typedef struct {

    u8   ch;
    u8   id;
    u8   type;
    u8   flags;
    u32  sec;
    u32  usec;
    u16  x;
    u16  y;
    u16  width;
    u16  height;
    char plate[OBE_PLATE_LEN];

} ob_event_t;

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------

extern void *obe_open(const char *filename, int readonly, u32 start, u32 duration);
extern int   obe_close(void *handle);
extern int   obe_write(void *handle, ob_event_t *ev);
extern int   obe_read(void *handle, u8 ch, u8 id, u8 type, ob_event_t *ev);
extern int   obe_seek(void *handle, u32 ts);
extern int   obe_info(void *handle, u32 *start, u32 *end);
extern int   obe_set_dir(void *handle, int is_backward);

#endif // __OB_EVENT_H__

