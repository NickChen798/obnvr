
#ifndef __LPR_H__
#define __LPR_H__

extern void *lpr_alloc(short width, short height);
extern void  lpr_free(void *handle);
extern int   lpr_process(void *handle, void *buf);
extern int   lpr_dump_result(void *handle);

#endif // __LPR_H__

