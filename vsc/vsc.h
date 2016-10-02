
#ifndef __VSC_H__
#define __VSC_H__

#include "as_protocol.h"

typedef int (*vsc_callback_t) (void *, void *, u8 *);

extern void *vsc_alloc(u32 ip, u8 ch, u8 id, u8 mode, int port, vsc_callback_t cb, void *priv);
extern int   vsc_free(void *handle);
void		Ask_Vs_State(int *Vs_state);

#endif // __VSC_H__

