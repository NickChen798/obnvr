#include "ob_platform.h"
#include "codec.h"
#include "vsc/vsc.h"

static void *vsc_handle = 0;
#define BSIZE 0x30000000
static u8 *jpeg_buf = 0;

#define WRITE2FILE
static int jpeg_count = 0;

static int vsc_callback(void *priv, void *header, u8 *data)
{
    as_frame_t *fh = (as_frame_t *)header;
    //dbg("%u bytes (%c)\n", fh->size, (fh->flags & 0x80) ? 'I' : 'P');
    if(fh->sub_type == ASF_VTYPE_H264) {
        if(!jpeg_buf)
            jpeg_buf = (u8 *)malloc(BSIZE);
        if(fh->flags & ASF_FLAG_KEY) {
            int psize = h264tojpeg(data, fh->size, jpeg_buf, BSIZE, 100, 100, 320, 240,0,0);
            dbg("jpeg = %d bytes\n", psize);
#ifdef WRITE2FILE
            char name[20];
            sprintf(name, "test-%04d.jpg", jpeg_count++);
            FILE *fp = fopen(name, "wb");
            if(fp) {
                fwrite(jpeg_buf, 1, psize, fp);
                fclose(fp);
            }
#endif
        }
    }
    return 0;
}

static void quit_signal(int sig)
{
	static int disable_flag = 0;

	if (!disable_flag) {
		disable_flag = 1;
	}

	msleep(100);

	exit(1);
}

int main(int argc, char **argv)
{
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);

    if(codec_init() < 0)
        exit(0);

    //vsc_handle = vsc_alloc((u32)ntohl(inet_addr("10.53.43.1")), 0, 0, 0, vsc_callback, 0);

	as_frame_t *fh ;
	FILE *fp ;
	u8 *data ;
	//dbg("%u bytes (%c)\n", fh->size, (fh->flags & 0x80) ? 'I' : 'P');
		jpeg_buf = (u8 *)malloc(BSIZE);

		fp = fopen("file.h264","rb");
		if (fp)
		{
			fread(data,1,1920*1080,fp);
			fclose(fp);
		}

			int psize = h264tojpeg(data, 1920*1080, jpeg_buf, BSIZE, 0, 0, 1920, 1080,0,0);
			dbg("jpeg = %d bytes\n", psize);

    while(vsc_handle) {
        sleep(1);
    }

	return 0;
}

