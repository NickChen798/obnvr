
#ifndef __CODEC_H__
#define __CODEC_H__

extern int  codec_init(void);
extern int  codec_exit(void);
extern int  h264tojpeg(void *data, int size, void *out, int bsize, int x, int y, int width, int height, int flag,int id);
#endif // __CODEC_H__

