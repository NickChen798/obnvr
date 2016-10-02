
#include "SocketServer.h"
#include "include/ob_platform.h"
#include "include/as_protocol.h"
#include "storage/ob_recorder.h"
#include "storage/ob_reader.h"
#include "codec/codec.h"

static	void			*hgc;

static void quit_signal(int sig)
{
	static int disable_flag = 0;

	if (!disable_flag) {
#ifdef ENABLE_WDT
        // disable WDT
#endif

		disable_flag = 1;
	}

	msleep(100);

	exit(1);
}

int Insert(void *hSock)
{
	return 0;	
}

static int sock_callback(BYTE *priv)
{
	return 0;
}

int main(int argc, char **argv)
{
	signal(SIGINT, quit_signal);

	int			base_no = 1;
	int parkingID = 1;

	pthread_mutex_t 	mux;

	result_info		res_list[MAX_CH];

	hgc = Create_client("192.168.1.221", 8201, "192.168.1.248");
	if(hgc){

		pthread_mutex_init(&(mux),NULL);

	int y;
	for (y=0 ; y<MAX_CH ; y++)
	{
		res_list[y].cout = 0;
		sprintf(res_list[y].plate, "0000");
	}

	Set_sck_param(hgc, parkingID, base_no,0, res_list, 0, &mux);

	struct timeval tt;
	u32 now;

	gettimeofday(&tt, 0);
	now = (u32) tt.tv_sec;

	sprintf(res_list[0].plate, "AB-1234");
	res_list[0].cout = 6;
	res_list[0].timestamp = now;
	res_list[0].micro_sec = 0;

	sprintf(res_list[1].plate, "CD-1234");
	res_list[1].cout = 5;
	res_list[1].timestamp = now;
	res_list[1].micro_sec = 1000;

	sprintf(res_list[2].plate, "EF-1234");
	res_list[2].cout = 4;
	res_list[2].timestamp = now;
	res_list[2].micro_sec = 2000;

	}

	Gateway_Connect(hgc);
	sleep(1);

	Set_flag(hgc, parkingID);
    
	while(1) {
        sleep(1);
    }

	return 0;
}

