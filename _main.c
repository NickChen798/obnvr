#include "ob_platform.h"
#include "lpr/lpr.h"
#include "vsc/vsc.h"

static int vsc_callback(void *priv, as_frame_t *fh, u8 *data)
{
    dbg("frame in\n");
    return 0;
}

static int lpr_test(void)
{
    void *lpr = lpr_alloc(320, 240);
    if(!lpr) {
        dbg("LPR alloc failed\n");
        exit(-1);
    }

    int ret;
	clock_t	start, finish;
    start = clock();
    ret = lpr_process(lpr, 0);
    finish = clock();

    if(ret == 0) {
        u32 elapsed = (u32)(finish - start) * 1000 / CLOCKS_PER_SEC;
        dbg("LPR takes %u msec\n", elapsed);
        lpr_dump_result(lpr);
    }
    else {
        dbg("LPR failed\n");
    }

    lpr_free(lpr);

    return 0;
}

int main(int argc, char **argv)
{
    void *handle = vsc_alloc((u32)ntohl(inet_addr("10.53.43.1")), 0, 0, 0, vsc_callback, 0);

    lpr_test();

	return 0;
}

