/*!
 *  @file       ob_file.h
 *  @version    1.0
 *  @date       04/01/2013
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       Record File Structure
 */

#ifndef __OB_FILE_H__
#define __OB_FILE_H__

#include "ob_platform.h"
#include "as_protocol.h"

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------

extern void *obf_open(const char *filename, int readonly, u32 start, u32 duration);
extern int   obf_close(void *handle);
extern int   obf_write(void *handle, as_frame_t *fh, u8 *data);
extern int   obf_read(void *handle, as_frame_t *fh, u8 *data, int bsize);
extern int   obf_seek(void *handle, u32 ts);
extern int   obf_info(void *handle, u32 *start, u32 *end);
extern int   obf_set_dir(void *handle, int is_backward);

#endif // __OB_FILE_H__

