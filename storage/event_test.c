#include "ob_platform.h"
#include "ob_recorder.h"
#include "ob_reader.h"
#include "codec/codec.h"

static void search(int ch, int id)
{
#if 0
    time_t end = time(0);
    time_t start = end - 3600;
#else
    time_t end = 0x7fffffff;
    time_t start = 0;
#endif
    void *handle = 0;
    u64 t0, t1;

    dbg("\n////////////////////////////////////////\n\n");
    //////////////////////////////////////////////
    t0 = ob_msec();
    dbg("Search VLOSS start @ %qu.%03qu\n", t0/1000, t0%1000);

    handle = obr_alloc(ch, id, start, end);

    if(handle) {
        obr_result_t result;
        obr_setup(handle, OBR_EV_VLOSS, OBR_IMG_NONE, OBR_ORDER_ASCENT);
        while(obr_read(handle, &result) == 0) {
            dbg("RESULT : ch#%d VLOSS @ %u.%06u\n", result.ch, (u32)result.sec, (u32)result.usec);
        }
        obr_free(handle);
    }
    else
        dbg("alloc reader failed\n");

    obr_free(handle);

    t1 = ob_msec();
    dbg("Search VLOSS end @ %qu.%03qu (%qu msec)\n", t1/1000, t1%1000, t1-t0);

    dbg("\n////////////////////////////////////////\n\n");
    //////////////////////////////////////////////
    t0 = ob_msec();
    dbg("Search VRECOVERY start @ %qu.%03qu\n", t0/1000, t0%1000);

    handle = obr_alloc(ch, id, start, end);

    if(handle) {
        obr_result_t result;
        obr_setup(handle, OBR_EV_VRECOVERY, OBR_IMG_NONE, OBR_ORDER_DESCENT);
        while(obr_read(handle, &result) == 0) {
            dbg("RESULT : ch#%d VRECOVERY @ %u.%06u\n", result.ch, (u32)result.sec, (u32)result.usec);
        }
        obr_free(handle);
    }
    else
        dbg("alloc reader failed\n");

    obr_free(handle);

    t1 = ob_msec();
    dbg("Search VRECOVERY end @ %qu.%03qu (%qu msec)\n", t1/1000, t1%1000, t1-t0);

    dbg("\n////////////////////////////////////////\n\n");
    //////////////////////////////////////////////
    t0 = ob_msec();
    dbg("Search Plate start @ %qu.%03qu\n", t0/1000, t0%1000);

    handle = obr_alloc(ch, id, start, end);

    if(handle) {
        obr_result_t result;
        obr_setup(handle, OBR_EV_PLATE, OBR_IMG_FULL, OBR_ORDER_ASCENT);
        while(obr_read(handle, &result) == 0) {
            if(result.data_type != OBR_DT_NONE) {
                dbg("RESULT : %u.%06u, %s, (%u x %u) %u bytes (ch#%d)\n", (u32)result.sec, (u32)result.usec, result.plate, result.width, result.height, result.size, result.ch);
            }
            else {
                dbg("RESULT : %u.%06u, %s (ch#%d)\n", (u32)result.sec, (u32)result.usec, result.plate, result.ch);
            }
        }
        obr_free(handle);
    }
    else
        dbg("alloc reader failed\n");

    obr_free(handle);

    t1 = ob_msec();
    dbg("Search Plate end @ %qu.%03qu (%qu msec)\n", t1/1000, t1%1000, t1-t0);

    dbg("\n////////////////////////////////////////\n\n");
    //////////////////////////////////////////////
    t0 = ob_msec();
    dbg("Search Plate start @ %qu.%03qu\n", t0/1000, t0%1000);

    handle = obr_alloc(ch, id, start, end);

    if(handle) {
        obr_result_t result;
        obr_setup(handle, OBR_EV_PLATE, OBR_IMG_PLATE, OBR_ORDER_DESCENT);
        while(obr_read(handle, &result) == 0) {
            if(result.data_type != OBR_DT_NONE) {
                dbg("RESULT : %u.%06u, %s, (%u x %u) %u bytes (ch#%d)\n", (u32)result.sec, (u32)result.usec, result.plate, result.width, result.height, result.size, result.ch);
            }
            else {
                dbg("RESULT : %u.%06u, %s (ch#%d)\n", (u32)result.sec, (u32)result.usec, result.plate, result.ch);
            }
        }
        obr_free(handle);
    }
    else
        dbg("alloc reader failed\n");

    obr_free(handle);

    t1 = ob_msec();
    dbg("Search Plate end @ %qu.%03qu (%qu msec)\n", t1/1000, t1%1000, t1-t0);

    dbg("\n////////////////////////////////////////\n\n");
}

int main(int argc, char **argv)
{
    codec_init();

    rec_config_t rc;
    strcpy(rc.path, "/mnt/data");
    rc.ch_num = 2;
    rc.ch_info[0].ip = ntohl(inet_addr("10.53.43.1"));
    rc.ch_info[0].ch = 0;
    rc.ch_info[0].id = 0;
    rc.ch_info[1].ip = ntohl(inet_addr("10.53.43.1"));
    rc.ch_info[1].ch = 0;
    rc.ch_info[1].id = 1;

    rec_init(&rc);

    sleep(1);

#define CH_NR 2
#define VLOSS 0
    int vl_idx = 0;
    int pl_idx = 0;
    int vloss[2] = {0,0};
    int plate_on[2] = {1, 1};
    int plate[2] = {0, 5000};
    u32 last = 0;

    while(1) {

        msleep(200);

        struct timeval tt;
        gettimeofday(&tt, 0);
        u32 now = (u32) tt.tv_sec;

        if(now == last)
            continue;
#if VLOSS > 0
        if((now % 10) == 0) {
            rec_event_video(vl_idx, vloss[vl_idx]);
            vloss[vl_idx] ^= 1;
            vl_idx = (vl_idx+1) % CH_NR;
        }
#endif
        if((now % 30) == 0) {
            if(plate_on[pl_idx]) {
                char plate_str[20];
                sprintf(plate_str, "A1-%04d", plate[pl_idx]);
                rec_event_plate(pl_idx, 0, plate_str, now, (u32)tt.tv_usec, 100, 100, 320, 240);
                plate[pl_idx] = (plate[pl_idx] + 1) % 10000;
            }
            else {
                rec_event_plate(pl_idx, 0, 0, now, (u32)tt.tv_usec, 0, 0, 0, 0);
            }
            plate_on[pl_idx] ^= 1;
            pl_idx = (pl_idx+1) % CH_NR;
        }

        if((now % 60) == 0) {
            search(0, 0); // search all ch
        }

        last = now;
    }

    return 0;
}

