#include "ob_platform.h"
#include "ob_recorder.h"
#include "ob_reader.h"
#include "codec/codec.h"

int main(int argc, char **argv)
{
/*    codec_init();

    rec_config_t rc;
    strcpy(rc.path, "/mnt/data");
    rc.ch_num = 1;
    rc.ch_info[0].ip = ntohl(inet_addr("10.53.43.1"));
    rc.ch_info[0].ch = 0;
    rc.ch_info[0].id = 0;

    rec_init(&rc);

    sleep(10);

    u32 now = (u32) time(0);
    void *handle;
    FILE *fp = 0;

    // Read video (ascent)
    dbg("Reader test 1 start \n");

    handle = obr_alloc(0, 0, now-6, now-1);
    fp = fopen("test1.h264", "wb");

    if(handle) {
        obr_result_t result;
        obr_setup(handle, OBR_EV_VIDEO, OBR_IMG_NONE, OBR_ORDER_ASCENT);
        while(obr_read(handle, &result) == 0) {
            if(fp)
                fwrite(result.buffer, 1, result.size, fp);
            dbg("RESULT : %dx%d, %u bytes @ %u.%06u\n", result.width, result.height, result.size, (u32)result.sec, (u32)result.usec);
        }
        obr_free(handle);
    }
    else
        dbg("alloc reader failed\n");

    if(fp)
        fclose(fp);
    obr_free(handle);

    dbg("Reader test 1 end\n");

    // Read Video (descent)
    dbg("Reader test 2 start \n");

    handle = obr_alloc(0, 0, now-6, now-1);
    fp = fopen("test2.h264", "wb");

    if(handle) {
        obr_result_t result;
        obr_setup(handle, OBR_EV_VIDEO, OBR_IMG_NONE, OBR_ORDER_DESCENT);
        while(obr_read(handle, &result) == 0) {
            if(fp)
                fwrite(result.buffer, 1, result.size, fp);
            dbg("RESULT : %dx%d, %u bytes @ %u.%06u\n", result.width, result.height, result.size, (u32)result.sec, (u32)result.usec);
        }
        obr_free(handle);
    }
    else
        dbg("alloc reader failed\n");

    if(fp)
        fclose(fp);
    obr_free(handle);

    dbg("Reader test 2 end\n");

    // Snapshot
    dbg("Reader test 3 start \n");

    void *jbuf = malloc(200000);
    if(jbuf) {
        now = (u32) time(0);
        int jsize = obr_snapshot(0, now, jbuf, 200000, 0, 0, 0, 0);
        dbg("jpg size = %d\n", jsize);
        if(jsize > 0) {
            fp = fopen("test3.jpg", "wb");
            if(fp) {
                fwrite(jbuf, 1, jsize, fp);
                fclose(fp);
            }
        }
        free(jbuf);
    }

    dbg("Reader test 3 end\n");
*/

	u32 now = (u32) time(0);
	void *handle;
	int	x=0;

    while(1) {		
		printf("[DBG] Reader test  start \n");
		// Read Video (descent)	
		handle = obr_alloc(0, 0, now-(3600*x), now-(3600*(x+1)));

		if(handle) {
			obr_result_t result;
			//obr_setup(handle, OBR_EV_VIDEO, OBR_IMG_NONE, OBR_ORDER_DESCENT);
			obr_setup(handle, OBR_EV_PLATE, OBR_IMG_NONE, OBR_ORDER_DESCENT);
			while(obr_read(handle, &result) == 0) 
			{
				printf("[DBG] recv time=%d, usec=%d \n", result.sec, result.usec);
			}
			obr_free(handle);
		}
		else
			dbg("alloc reader failed\n");

		printf("[DBG] free handle. \n");
		obr_free(handle);

		x++;
		sleep(2);
    }

	return 0;
}

