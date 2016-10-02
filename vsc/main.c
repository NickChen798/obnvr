#include "ob_platform.h"
#include "lpr/lpr.h"
#include "vsc.h"

typedef struct {
    int     id;
    char    vs1_ip[20];
    void   *vsc;
    void   *lpr;
    u16     width;
    u16     height;
} peer_t;

static peer_t peers[] = {
    {0, "192.168.1.215", NULL, NULL, 0, 0},
    {1, "0.0.0.0", NULL, NULL, 0, 0}
};
#define PEER_NR     (sizeof(peers)/sizeof(peers[0]))

static void quit_signal(int sig)
{
	msleep(100);

	exit(1);
}

static int vsc_callback(void *priv, void *header, u8 *data)
{
    peer_t *p = (peer_t *)priv;
    as_frame_t *fh = (as_frame_t *)header;
    if(p->width != fh->width || p->height != fh->height || p->lpr == NULL) {
        if(p->lpr)
            lpr_free(p->lpr);
       /// p->lpr = lpr_alloc(fh->height, fh->width);
        if(!p->lpr) {
            dbg("#%d lpr alloc failed\n", p->id);
            return -1;
        }
        p->width = fh->width;
        p->height = fh->height;
        dbg("New LPR %u x %u\n", fh->width, fh->height);
    }

    if(fh->size == 0 || data == NULL) {
        dbg("#%d-%d No Car\n", p->id, fh->padding); // fh->padding = ROI id
        return 0;
    }

    int nr = fh->size / (fh->width*fh->height);
    int i;
    for(i=0;i<nr;i++) {
        int ret;
        clock_t	start, finish;
        start = clock();
        //ret = lpr_process(p->lpr, data+i*fh->width*fh->height);
        finish = clock();

        if(ret == 0) {
            u32 elapsed = (u32)(finish - start) * 1000 / CLOCKS_PER_SEC;
            dbg("#%d-%d frame#%d LPR takes %u msec\n", p->id, fh->padding, i, elapsed);
            //lpr_dump_result(p->lpr);
        }
        else {
            dbg("#%d-%d frame#%d LPR failed\n", p->id, fh->padding, i);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);

    int i;
    for(i=0;i<PEER_NR;i++) {
        u32 ip = (u32)ntohl(inet_addr(peers[i].vs1_ip));
        if(!ip)
            break;
        peers[i].vsc = vsc_alloc(ip, 0, 0, 1, vsc_callback, &peers[i]);
        if(!peers[i].vsc) {
            dbg("#%d vsc alloc falied\n", i);
            break;
        }
    }

    while(1) {
        sleep(1);
    }

	return 0;
}

