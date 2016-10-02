#include "ob_platform.h"
#include "cexcept.h"
#include "as_protocol.h"
#include "lpr/lpr.h"
#include "vsc/vsc.h"
#include "ipc/SocketServer.h"
#include "storage/ob_reader.h"
#include "storage/ob_recorder.h"
#include "codec/codec.h"
#include "mcu/mcu.h"
#include "dbg_msg/dbg_log.h"

#include <pthread.h>
#include <semaphore.h>

#define BSIZE       0x200000
#define INI			"3531.ini"

#define WDT_ENABLE
#define WDT_MARGIN_TIME		60
//#define MAX_LPR_COUNT	50

#ifndef MAX_CH
#define MAX_CH		32
#endif

typedef struct	
{
	int					ch_id;
	void				*lpr;

	as_yframe_t			fh;

	int					parkingID;

	int					state;
	int					confidence;
	int					buf_flag;
	int					lost_event_times;
	int					no_ans;
	
	int					top_cout;
	int					base_cout;
	int					open_flag;
	int					max_lpr_counter;
		
	sem_t				case_evnet;
	pthread_mutex_t 	thread_mutex;

	u16					width;          // bitwidth for audio
	u16					height;         // sample rate for audio

	u32					timestamp;
	u32					micro_sec;

	result_info			ev_empty;
	result_info			voting_ans;
	result_info			result_list[MAX_CH];
 
	u8					*image_buf;
	//int					control_dbg;
}lpr_info;


typedef struct	
{
	int					ch_id;
	u32					ip;
	int					port;
	//int					parkingID[2];

	void				*vsc;
	lpr_info			handle_lpr[4];

}channel_info;

// -------------------------------------------------------------------------------------
static void				*socket_ipc;

static channel_info		vsc_lpr[MAX_CH];

static int              total_ch;

static rec_config_t		rc;

static void				*hgc;

static char				gw_ip[32];

static char				loc_ip[32];

static char				log_msg[256];

static int				rec_fps;

static int				park_list[32];


// -------------------------------------------------------------------------------------
static void quit_signal(int sig)
{
	int ret,open;
	char tmp[32];
	ret = GetPrivateProfileString( "System", "Debug_Mod", "0", tmp, 32, INI );
	open = atoi(tmp);
	//printf("[QUIT] get quit signal");
	Write_Log(0,"[QUIT] get quit signal");
	if (open == 1)
	{
		Write_Log(0,"[QUIT] Debugmod");
		//if (mcu_wdt_disable() < 0) {
		//	dbg("wdt_disable error\n");
		//}
	}
	

	/*static int disable_flag = 0;

	if (!disable_flag) {
		if (mcu_wdt_disable() < 0) {
			dbg("wdt_disable error\n");
		}
		else {
			dbg("WDT disabled\n");
		}

		disable_flag = 1;
	}*/

	usleep(100000);

	exit(1);
}

static int mcu_callback(int id, int data)
{
	dbg("CALLBACK: id=%d, data=%d\n", id, data);
	return 0;
}

static void clean_dbg(char *path)
{
	
}

static int get_vsc_list(int channel_id, channel_info* ch_info, char* file_name)
{
	int			ret;
	int			id;
	int			roi;
	int			port;
	char		section[32];
	char		tmp[32];
	u32			ip;

	int			x;

	memset(section, 0 ,32);
	memset(tmp, 0 ,32);

	sprintf(log_msg,"[MAIN] [PARKINGLIST] ch_id=%d, filename=%s .\n", channel_id+1, file_name );
	Write_Log(0,log_msg);
	sprintf(section, "Parking%d", channel_id+1 );
	ret = GetPrivateProfileString( section, "IP_Addr", "0.0.0.0", tmp, 32, file_name );
	ip = ((u32)(ntohl(inet_addr(tmp))));

	ret = GetPrivateProfileString( section, "Parking_ID", "0", tmp, 32, file_name );
	id = atoi(tmp);

	if(id==0){id=channel_id+1;}

	ret = GetPrivateProfileString( section, "ROI_Index", "0", tmp, 32, file_name );
	roi = atoi(tmp);

	ret = GetPrivateProfileString( section, "ROI_Port", "8010", tmp, 32, file_name );
	port = atoi(tmp);

	sprintf(log_msg,"[MAIN] [PARKINGLIST] get list %d times, ip=%d, park=%d, roi=%d.\n", channel_id+1,  ip, id, roi);
	Write_Log(1,log_msg);

	if(ip != 0)
	{
		sprintf(log_msg,"[MAIN] [PARKINGLIST] get into find list \n");
		Write_Log(1,log_msg);
		for(x=0 ; x<32 ; x++ )
		{
		
			sprintf(log_msg,"[MAIN] [PARKINGLIST] find list[%d] ip=%d\n", x, ch_info[x].ip);
			Write_Log(1,log_msg);
			if (ch_info[x].ip == ip)
			{
				if (port == 8011)
				{roi = roi + 2;}
				
				sprintf(log_msg,"[MAIN] [PARKINGLIST] ip=%d find in list[%d] port = %d\n", ip ,x ,port);
				Write_Log(0,log_msg);
				if( ch_info[x].handle_lpr[roi-1].parkingID != -1 )
				{//已有設定過
					sprintf(log_msg,"[MAIN] [PARKINGLIST] [ERROR] roi_index[%d] is used \n", roi );
					Write_Log(0,log_msg);
					return 0-channel_id;
				}
				else
				{
					//ch_info[x].handle_lpr[roi-1].ch_id = 
					ch_info[x].handle_lpr[roi-1].parkingID = id;
					ch_info[x].port = port;
					sprintf(log_msg,"[MAIN] [PARKINGLIST] list[%d] roi_index[%d] set %d \n", x, roi, ch_info[x].handle_lpr[roi-1].parkingID );
					Write_Log(1,log_msg);
					break;
				}
			}
			else if(ch_info[x].ip == 0)
			{//ip not find
				ch_info[x].ip = ip;
				ch_info[x].port = port;
				ch_info[x].handle_lpr[roi-1].parkingID = id;

				sprintf(log_msg,"[MAIN] [PARKINGLIST] add in list[%d] ip=%d  ,roi=%d, park=%d.\n", x, ip, roi, ch_info[x].handle_lpr[roi-1].parkingID);
				Write_Log(0,log_msg);
				total_ch = total_ch + 1;
				break;
			}	
		}
	}
	else
	{
		sprintf(log_msg,"[MAIN] [PARKINGLIST] not use ch%04d \n", channel_id+1);
		Write_Log(0,log_msg);
		return -1;
	}

	return channel_id;
}

static int vsc_callback(void *priv, void *fh, u8 *data)
{
	/*
	typedef struct __attribute__ ((packed)) {

	u32     magic;
	u32     size;
	u32     sec;
	u32     usec;
	u8      id;             // ROI id
	u8      flags;
	u8      resv[2];
	u16     width;          // bitwidth for audio
	u16     height;         // sample rate for audio
	u16     x;
	u16     y;

	} as_yframe_t;
	*/
	channel_info*	wPriv = (channel_info*) priv;
	as_yframe_t*	wfh = (as_yframe_t*) fh;
	struct timeval tt;
	gettimeofday(&tt, 0);
	u32 now = (u32) tt.tv_sec;

	///FILE *fp;

	//fp = fopen("/app/msic/test.obi","wb");
	//if (fp)
	//{Write_Log(1,"writefile\n");
	//	fwrite(data,1,320*240,fp);
	//	fclose(fp);
	//}

	sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] vsc%d get padding%d callback size=%d \n", wPriv->handle_lpr[wfh->id].parkingID , wPriv->ch_id+1, wfh->id, wfh->size);
	Write_Log(1,log_msg);
	sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] vsc%d -- lpr parkID[%d]=%d. \n", wPriv->handle_lpr[wfh->id].parkingID , wPriv->ch_id+1, wfh->id, wPriv->handle_lpr[wfh->id].parkingID );
	Write_Log(1,log_msg);
    //dbg("frame in\n");
	if(wPriv->handle_lpr[wfh->id].parkingID != -1)
	{
		if (wfh->size != 0)
		{
			sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] get event ,tag=%d size=%d \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1, wfh->id, wfh->size);
			Write_Log(1,log_msg);
			if (wPriv->handle_lpr[wfh->id].state == 0)
			{
				rc.ch_info[park_list[wPriv->handle_lpr[wfh->id].parkingID-1]].rec->motion_state=1;
				sprintf(log_msg,"[MAIN] [Motion] %d \n",rc.ch_info[park_list[wPriv->handle_lpr[wfh->id].parkingID-1]].ip);
				Write_Log(0,log_msg);
			}		
			wPriv->handle_lpr[wfh->id].state = 1;

			//pthread_mutex_lock(&(wPriv->handle_lpr[wfh->id].thread_mutex));
			if (wPriv->handle_lpr[wfh->id].buf_flag == 0)
			{
				wPriv->handle_lpr[wfh->id].buf_flag = 1;

				//複製資料到辨識的參數
				memcpy(&(wPriv->handle_lpr[wfh->id].fh), fh, sizeof(as_yframe_t));
				memcpy( wPriv->handle_lpr[wfh->id].image_buf, data, wfh->size);
				wPriv->handle_lpr[wfh->id].timestamp = now;
				wPriv->handle_lpr[wfh->id].micro_sec = (u32)tt.tv_usec;

				sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] get event, now=%d, usec=%d \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1, wPriv->handle_lpr[wfh->id].timestamp, wPriv->handle_lpr[wfh->id].micro_sec);
				Write_Log(1,log_msg);
				wPriv->handle_lpr[wfh->id].buf_flag = 0;

				sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] - send lpr event .\n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1 );
				Write_Log(0,log_msg);
				sem_post( &(wPriv->handle_lpr[wfh->id].case_evnet) );
				sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] - send over \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1 );
				Write_Log(0,log_msg);
			}
			else
			{
				wPriv->handle_lpr[wfh->id].lost_event_times++;
				sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] [WARNING] callback-vsc[%d] - lost times = %d \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1, wPriv->handle_lpr[wfh->id].lost_event_times);
				Write_Log(0,log_msg);
			}
			//pthread_mutex_unlock(&(wPriv->handle_lpr[wfh->id].thread_mutex));
		}
		else 
		{
			//printf("[callback] vsc[%d] ready insert park%d empty event . \n", wPriv->ch_id+1, wPriv->parkingID[wfh->id]);
			if (wPriv->handle_lpr[wfh->id].buf_flag == 0)
			{
				wPriv->handle_lpr[wfh->id].buf_flag = 1;
				if( wPriv->handle_lpr[wfh->id].state != 0)
				{
					rc.ch_info[park_list[wPriv->handle_lpr[wfh->id].parkingID-1]].rec->motion_state=1;
					
					wPriv->handle_lpr[wfh->id].state = 0;
					sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] insert - 0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
					Write_Log(1,log_msg);
					sprintf(log_msg,"[MAIN] [Motion] %d \n",rc.ch_info[park_list[wPriv->handle_lpr[wfh->id].parkingID-1]].ip);
					Write_Log(0,log_msg);
					rec_event_plate(wPriv->ch_id, wPriv->handle_lpr[wfh->id].parkingID, 0, now, (u32)tt.tv_usec, 0, 0, 0, 0);
					//sprintf(log_msg,"[RECEVENT] [AFTER] rec_fps_temp = %d , rec_fps = %d\n",rec_fps_temp,rec_fps);
					//Write_Log(0,log_msg);				
					//if ((rec_fps_temp-1)==0)
					//{
					//	rec_event_plate(wPriv->ch_id, wPriv->handle_lpr[wfh->id].parkingID, 0, now, (u32)tt.tv_usec, 0, 0, 0, 0);
					//	rec_fps_temp = rec_fps;
					//	sprintf(log_msg,"[RECEVENT] [MID] rec_fps_temp = %d , rec_fps = %d\n",rec_fps_temp,rec_fps);
					//	Write_Log(0,log_msg);
					//}
					//else
					//{
					//	rec_fps_temp = rec_fps_temp-1;
					//	sprintf(log_msg,"[RECEVENT] [BeFORE] rec_fps_temp = %d , rec_fps = %d\n",rec_fps_temp,rec_fps);
					//	Write_Log(0,log_msg);
					//}
					

					//pthread_mutex_lock(&(wPriv->handle_lpr[wfh->id].thread_mutex));
					sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] clean lpr list - 0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
					Write_Log(1,log_msg);
					wPriv->handle_lpr[wfh->id].max_lpr_counter = 0;
					wPriv->handle_lpr[wfh->id].no_ans = 0;
					wPriv->handle_lpr[wfh->id].ev_empty.timestamp = now;
					wPriv->handle_lpr[wfh->id].ev_empty.micro_sec = (u32)tt.tv_usec;
					Set_max(hgc,wPriv->handle_lpr[wfh->id].parkingID,1);
					lpr_list_clean(wPriv->handle_lpr[wfh->id].lpr, wPriv->handle_lpr[wfh->id].parkingID, 0);
					Clean_Answer_List(wPriv->handle_lpr[wfh->id].parkingID, now, (u32)tt.tv_usec);
					//pthread_mutex_unlock(&(wPriv->handle_lpr[wfh->id].thread_mutex));
					sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-send empty. \n", wPriv->handle_lpr[wfh->id].parkingID);
					Write_Log(0,log_msg);
					//Send_List(hgc, wPriv->parkingID[wfh->id], 0, 0, 0, 0, loc_ip, 1);
					
					Set_flag(hgc, wPriv->handle_lpr[wfh->id].parkingID);

					sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] clean over - 0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
					Write_Log(1,log_msg);
					//InsertAnswer(socket_ipc, wPriv->parkingID[wfh->id], "0000", data);
				}
				else
					{
						sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] state=0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
						Write_Log(1,log_msg);
					}
					
				wPriv->handle_lpr[wfh->id].buf_flag = 0;
			}
			else
				{
					sprintf(log_msg,"[MAIN] [VSC_callback] [ch%04d] callback-vsc[%d] set state=0000 fail. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
					Write_Log(0,log_msg);
				}
		}
	}

    return 0;
}



static int lpr_test(void *param)
{
	lpr_info	*wParam = (lpr_info*) param;

	int				frame_num;
    int				ret = 0;
	int				re_use = 0;
	clock_t			start, finish;

	int				sort_list[MAX_CH];
	char			tmp_plate[32];
	char			path[256];
	BYTE			tmp_buf[76800];
	
	int				send_ret = 0;

	//struct timeval tt;
	//gettimeofday(&tt, 0);
	//u32 now = (u32) tt.tv_sec;

	sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] start lpr function \n", wParam->parkingID);
	Write_Log(0,log_msg);
	sprintf(tmp_plate, "0000");

	while(1)
	{
		sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] wait vsc callback event \n", wParam->parkingID);
		Write_Log(1,log_msg);
		//lpr_list_clean(wParam->handle_lpr[0].lpr);
		sem_wait( &(wParam->case_evnet) );
		if(wParam->max_lpr_counter == 0){sprintf(tmp_plate, "0000");}
		sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] tmp_plate = %s \n", wParam->parkingID,tmp_plate);
		Write_Log(1,log_msg);
		if(wParam->max_lpr_counter >= wParam->top_cout)	//某一答案達到上限設定值
		{
			if(wParam->open_flag == 1)
			{	
				sprintf(log_msg,"[MAIN] [LPR_test] Before In Set_max %s.\n",tmp_plate);
				Write_Log(0,log_msg);
				Set_max(hgc, wParam->parkingID,2);
				lpr_list_clean(wParam->lpr, wParam->parkingID, 1);
				sprintf(log_msg,"[MAIN] [LPR_test] lpr_list_clean \n");
				Write_Log(0,log_msg);
				wParam->max_lpr_counter = 0;
				//sprintf(tmp_plate, "0000");
			}
			else
			{
				sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] max(%d) is enough(%d) ,stop lpr \n", wParam->parkingID, wParam->max_lpr_counter, wParam->top_cout);
				Write_Log(0,log_msg);
				//sleep(1);
				continue;
			}
		}

		frame_num = wParam->fh.size / (wParam->fh.width*wParam->fh.height);
		sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] get vsc callback event ,fh size=%d ,frame num=%d \n", wParam->parkingID, wParam->fh.size, frame_num);
		Write_Log(0,log_msg);
		if( wParam->buf_flag == 1)
		{
			sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] try to lock img-buffer fail", wParam->parkingID);
			Write_Log(0,log_msg);
		}
		else
		{
			wParam->buf_flag = 1;
			//pthread_mutex_lock(&(wParam->thread_mutex));
			for (re_use=0 ; re_use<frame_num ; re_use++)
			{
				start = clock();
				sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] send frame%d into lpr (%d) - buf(%X) \n", wParam->parkingID , re_use+1, wParam->lpr, wParam->image_buf+(wParam->fh.width*wParam->fh.height*re_use));
				Write_Log(1,log_msg);
				//sprintf(path, "//dbg//test%d.obi", wParam->parkingID);
				//RAW2File(path, wParam->image_buf+(wParam->fh.width*wParam->fh.height*re_use), 320, 240, 8, IMG_FMT_OBI, 0);
				ret = lpr_process(wParam->lpr, wParam->image_buf+(wParam->fh.width*wParam->fh.height*re_use), wParam->timestamp, wParam->micro_sec);
				//ret = 0;
			}
			//pthread_mutex_unlock(&(wParam->thread_mutex));

			finish = clock();

			if(ret == 0) 
			{
				u32 elapsed = (u32)(finish - start) * 1000 / CLOCKS_PER_SEC;
			
				sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] LPR takes %u msec\n", wParam->parkingID ,elapsed);
				Write_Log(1,log_msg);

				ret = lpr_dump_result(wParam->lpr, &(wParam->voting_ans), tmp_buf);
				//ret = 1;

				if(ret > 0)
				{
					sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] dump result is %s , ret = %d. \n", wParam->parkingID, wParam->voting_ans.plate, ret);
					Write_Log(0,log_msg);
					//printf("[XXX] 001 (ch%d)-%ld. \n", wParam->parkingID,wParam->result_list);
					if( (strcmp(tmp_plate, wParam->voting_ans.plate)!=0)) //答案不重複
					{
						rec_event_plate(wParam->ch_id, wParam->parkingID, wParam->voting_ans.plate, wParam->voting_ans.timestamp, wParam->voting_ans.micro_sec, wParam->fh.x+wParam->voting_ans.x1, wParam->fh.y+wParam->voting_ans.y1, wParam->voting_ans.x2-wParam->voting_ans.x1, wParam->voting_ans.y2-wParam->voting_ans.y1);
						//sprintf(log_msg,"[RECEVENT] [AFTER] rec_fps_temp = %d , rec_fps = %d\n",rec_fps_temp,rec_fps);
						//Write_Log(0,log_msg);
						//if ((rec_fps_temp-1)==0)
						//{
						//	rec_event_plate(wParam->ch_id, wParam->parkingID, wParam->voting_ans.plate, wParam->voting_ans.timestamp, wParam->voting_ans.micro_sec, wParam->fh.x+wParam->voting_ans.x1, wParam->fh.y+wParam->voting_ans.y1, wParam->voting_ans.x2-wParam->voting_ans.x1, wParam->voting_ans.y2-wParam->voting_ans.y1);
						//	rec_fps_temp = rec_fps;
						//	sprintf(log_msg,"[RECEVENT] [MID] rec_fps_temp = %d , rec_fps = %d\n",rec_fps_temp,rec_fps);
						//	Write_Log(0,log_msg);
						//}
						//else
						//{
						//	rec_fps_temp = rec_fps_temp-1;
						//	sprintf(log_msg,"[RECEVENT] [BEFORE] rec_fps_temp = %d , rec_fps = %d\n",rec_fps_temp,rec_fps);
						//	Write_Log(0,log_msg);
						//}

						//Insert_Answer_List(wParam->parkingID[0], wParam->handle_lpr[0].voting_ans.plate, wParam->handle_lpr[0].voting_ans.timestamp, wParam->handle_lpr[0].voting_ans.micro_sec);

						//memset(sort_list, 0 ,MAX_CH);
						//lpr_list_sort(wParam->parkingID[0], sort_list, wParam->handle_lpr[0].result_list);

						Set_flag(hgc, wParam->parkingID);
						//Send_List(hgc, wParam->parkingID[0], wParam->handle_lpr[0].base_cout, sort_list, (void *)wParam->handlse_lpr[0].result_list, &(wParam->handle_lpr[0].ev_empty), loc_ip, 0);

						sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] Send ret=%d", wParam->parkingID, ret);
						Write_Log(0,log_msg);

						strcpy(tmp_plate, wParam->voting_ans.plate);
					}
					wParam->max_lpr_counter = ret;					
				}
				else if(ret == 0)//##-####
				{
					sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] dump result is ##-#### , ret = %d. \n", wParam->parkingID, ret);
					Write_Log(0,log_msg);
				}
				else
				{
					sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d][ERROR] lpr dump result error\n", wParam->parkingID);
					Write_Log(0,log_msg);
				}
				sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] tmp_plate is %s , conter = %d. \n", wParam->parkingID, tmp_plate, wParam->max_lpr_counter);
				Write_Log(0,log_msg);
			}
			else 
			{
				sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d]LPR fail. \n", wParam->parkingID);
				Write_Log(0,log_msg);
			}
			wParam->buf_flag = 0;
			sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] lpr event over. \n", wParam->parkingID);
			Write_Log(0,log_msg);
		}
		msleep(100);
	}
	
exit_lpr_test:

	sprintf(log_msg,"[MAIN] [LPR_test] [ch%04d] [FREE]free lpr ", wParam->parkingID);
	Write_Log(1,log_msg);
    lpr_free(wParam->lpr);


    return 0;
}

static int scan_excist(void *arg)
{
	system("top > /dbg/top.log");
	return 0;
}

/*static int lpr_test1(void *param)
{
	channel_info	*wParam = (channel_info*) param;

	int				frame_num;
	int				ret = 0;
	int				re_use = 0;
	clock_t			start, finish;

	char			tmp_plate[32];
	int				sort_list[MAX_CH];
	BYTE			tmp_buf[76800];

	int				send_ret = 0;

	//char			path[32];

	printf("[ch%d] start lpr function \n", wParam->parkingID[1]);
	sprintf(tmp_plate, "0000");

	while(1)
	{
		printf("[ch%d] wait vsc callback event \n", wParam->parkingID[1]);
		sem_wait( &(wParam->handle_lpr[1].case_evnet) );

		if(wParam->handle_lpr[1].max_lpr_counter >= wParam->handle_lpr[1].top_cout)	//某一答案達到上限設定值
		{
			printf("[ch%d] max(%d) is enough(%d) ,stop lpr \n", wParam->parkingID[1], wParam->handle_lpr[1].max_lpr_counter, wParam->handle_lpr[1].top_cout);
			//sleep(1);
			continue;
		}

		frame_num = wParam->handle_lpr[1].fh.size / (wParam->handle_lpr[1].fh.width*wParam->handle_lpr[1].fh.height);
		printf("[ch%d] get vsc callback event ,fh size=%d ,frame num=%d \n", wParam->parkingID[1], wParam->handle_lpr[1].fh.size, frame_num);

		if( wParam->handle_lpr[1].buf_flag == 1)
		{
			printf("[ch%d] try to lock img-buffer fail", wParam->ch_id+1);
		}
		else
		{
			wParam->handle_lpr[1].buf_flag = 1;
			for (re_use=0 ; re_use<frame_num ; re_use++)
			{
				start = clock();
				printf("[ch%d] send frame%d into lpr (%d) \n", wParam->parkingID[1], re_use+1, wParam->handle_lpr[1].lpr);
				//sprintf(path, "//mnt//dbg//test%d.obi", wParam->parkingID[1]);
				//RAW2File(path, wParam->handle_lpr[1].image_buf+(wParam->handle_lpr[1].fh.width*wParam->handle_lpr[1].fh.height*re_use), 320, 240, 8, IMG_FMT_OBI, 0);
				ret = lpr_process(wParam->handle_lpr[1].lpr, wParam->handle_lpr[1].image_buf+(wParam->handle_lpr[1].fh.width*wParam->handle_lpr[1].fh.height*re_use), wParam->handle_lpr[1].timestamp, wParam->handle_lpr[1].micro_sec);
			}

			finish = clock();

			if(ret == 0) 
			{
				u32 elapsed = (u32)(finish - start) * 1000 / CLOCKS_PER_SEC;
				printf("[ch%d] LPR takes %u msec\n", wParam->parkingID[1] ,elapsed);
				ret = lpr_dump_result(wParam->handle_lpr[1].lpr, &(wParam->handle_lpr[1].voting_ans), tmp_buf);

				if(ret > 0)
				{
					printf("[ch%d] dump result is %s , ret = %d. \n", wParam->parkingID[1], wParam->handle_lpr[1].voting_ans.plate, ret);
					if( (strcmp(tmp_plate, wParam->handle_lpr[1].voting_ans.plate)!=0) || send_ret<0 ) //答案不重複
					{
						rec_event_plate(wParam->ch_id, wParam->parkingID[1], wParam->handle_lpr[1].voting_ans.plate, wParam->handle_lpr[1].voting_ans.timestamp, wParam->handle_lpr[1].voting_ans.micro_sec, wParam->handle_lpr[1].fh.x+wParam->handle_lpr[1].voting_ans.x1, wParam->handle_lpr[1].fh.y+wParam->handle_lpr[1].voting_ans.y1, wParam->handle_lpr[1].voting_ans.x2-wParam->handle_lpr[1].voting_ans.x1, wParam->handle_lpr[1].voting_ans.y2-wParam->handle_lpr[1].voting_ans.y1);

					//Insert_Answer_List(wParam->parkingID[1], wParam->handle_lpr[1].voting_ans.plate, wParam->handle_lpr[1].voting_ans.timestamp, wParam->handle_lpr[1].voting_ans.micro_sec);

						memset(sort_list, 0 ,MAX_CH);
						lpr_list_sort(wParam->parkingID[1], sort_list, wParam->handle_lpr[1].result_list);

						//Send_List(hgc, wParam->parkingID[1], wParam->handle_lpr[1].base_cout, sort_list, (void *)wParam->handle_lpr[1].result_list, &(wParam->handle_lpr[1].ev_empty), loc_ip, 0); 

						strcpy(tmp_plate, wParam->handle_lpr[1].voting_ans.plate);
					}
					
					wParam->handle_lpr[1].max_lpr_counter = ret;
				}
				else if(ret == 0)//##-####(理論上只會有一次)
				{
					printf("[ch%d] dump result is ##-#### , ret = %d. \n", wParam->parkingID[1], ret);
				}
				else
				{
					printf("[ch%d][ERROR] lpr dump result error\n", wParam->parkingID[1]);
				}
				printf("[ch%d] tmp_plate is %s , conter = %d. \n", wParam->parkingID[1], tmp_plate, wParam->handle_lpr[1].max_lpr_counter);
			}
			else    
			{
				printf("[ch%d]LPR fail. \n", wParam->parkingID[1]);
			}
			wParam->handle_lpr[1].buf_flag = 0;
			printf("[ch%d] lpr event over. \n", wParam->parkingID[1]);
		}
	}

exit_lpr_test:

	lpr_free(wParam->handle_lpr[1].lpr);

	return 0;
}*/

static int sock_callback(BYTE *priv)
{
	return 0;
}

int main(int argc, char **argv)
{
	//////////////////////////////////////////////////////////////////////////
	// //test
	//char newline[256];
	/*F ILE *fd = popen("http://125.227.102.165/APP/iParkLocal/Gettime.php", "r");
	while((fgets(newline, 256, fd)) != NULL) 
	{
		printf("We've got a newline %s", newline);
	}
	pclose(fd);

	InetReadFile("http://125.227.102.165/APP/iParkLocal/Gettime.php", newline, 256);

	printf("[TEST] newline = %s .\n", newline);
	
	return 0;

	/////////////////////////////////////////////////////////////////////////*/
	//VERSION 4 目前出版							                       //
	//VERSION 5 增加VS2第二個port										   //
	//VERSION 6 降Buffer，一小時重開                                       //
	//VERSION 7 Motion Deter                                               //
	//                                                                     //
	/////////////////////////////////////////////////////////////////////////
	//create_thread(scan_excist,0);
	int					ch, ret, x,re,mkd;
	int					V_F_count = 0;
	int					R_count = 0;
	int					Vs_state[MAX_CH+1];
	char				section[16], tmp[64], time_path[128];
	char				time_buffer_now[128], path[16],time_tmp[16];
	while(1)
	{
		struct timeval tt;
		struct	tm *now;
		char	time_buffer_yes[64];
		char	time_year[64];
		int		year;

		ret = GetPrivateProfileString( "System", "GW_Server", "0", gw_ip, 32, INI );

		sprintf(time_path,"./ntpclient -s -t -i 10 -h %s",gw_ip);
		system(time_path);
		gettimeofday(&tt, NULL);
		now = localtime(&tt.tv_sec);

		strftime (time_buffer_now, 80, "%Y%m", now);
		strftime (time_year, 80, "%Y", now);
		strftime (time_tmp, 80, "%Y%m%d%H", now);

		year = atoi(time_year);
		if(year>=2015 && year<=2018){break;}
		system("echo ntp get time > ./boot.log");
	}
	
	/*sprintf(time_path,"%s.log",time_buffer_now);
	if (rename("boot.log",time_path)<0)
	{
		printf("%s\n",strerror(errno));	
	}
	sprintf(time_path,"mv %s.log /dbg/%s.log",time_buffer_now,time_buffer_now);
	system(time_path);*/
	system("echo start main obnvr>./boot.log");

	int muc_cout = 0;	
	if(mcu_init(mcu_callback) < 0)
		Write_Log(0,"[MCU] init fail\n");
	while(muc_cout < 20)
	{
		if( mcu_wdt_reset(WDT_MARGIN_TIME) < 0)
		{
			Write_Log(0,"[MCUE] reset fail\n");
			//printf("[MCU] reset fail");
			sleep(1);
			muc_cout++;
			if (muc_cout == 20)
			{
				system("reboot");
			}
		}
		else break;
	}
	system("echo mcu over>./boot.log");
	Write_Log(0,"==================OBNVR VERSION 7==================\n");
//CREATE DIR============================================================================
	/*if((re=remove("/dbg/lpr"))!=0)
	{
		Write_Log(0,"remove dbg/lpr fail\n");
		//printf("remove dbg/lpr fail\n");
	}*/
	if((mkd=mkdir("/dbg/lpr",S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
	{
		Write_Log(0,"[MAIN] [Init] mkdir dbg/lpr fail\n");
		//printf("mkdir dbg/lpr fail\n");
	}
		system("echo mkdir dbg/lpr fail>./boot.log");
	/*if((re=remove("/dbg/log"))!=0)
	{
		Write_Log(0,"remove dbg/log fail\n");
		//printf("remove dbg/log fail\n");
	}*/
	if((mkd=mkdir("/dbg/log",S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
	{
		Write_Log(0,"[MAIN] [Init] mkdir dbg/log fail\n");
		//printf("mkdir dbg/log fail\n");
	}
		system("echo mkdir dbg/log fail>./boot.log");
	/*if((re=remove("/dbg/vs"))!=0)
	{
		Write_Log(0,"remove dbg/vs fail\n");
		//printf("remove dbg/vs fail\n");
	}*/
	if((mkd=mkdir("/dbg/vs",S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
	{
		Write_Log(0,"[MAIN] [Init] mkdir dbg/vs fail\n");
		//printf("mkdir dbg/vs fail\n");
	}
		system("echo mkdir dbg/vs fail>./boot.log");
	sprintf(path,"/dbg/lpr/%s",time_tmp);
	if((mkd=mkdir(path,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
	{
		printf("[DBG] mkdir dbg/lpr/ fail\n");
	}
//=======================================================================================

	int					num = 0;
	int					port =0;
	int					Vs_port =0;
	ret = GetPrivateProfileString( "System", "TotalNum", "32", tmp, 32, INI );
	num = ((u32)(ntohl(inet_addr(tmp))));
	ret = GetPrivateProfileString( "System", "PortNum", "8201", tmp, 32, INI );
	port = atoi(tmp);
	ret = GetPrivateProfileString( "System", "FpsNum", "3", tmp, 32, INI );
	rec_fps = atoi(tmp);
	sprintf(log_msg,"[MAIN] [Init] get total num = %d, get port num = %d, get fps num = %d. \n", num, port, rec_fps);
	Write_Log(0,log_msg);
	//printf("[Init] get total num = %d. \n", num);

	//printf( "init vsc_lpr enable & list. \n" );
	Write_Log(0,"[MAIN] [Init] init vsc_lpr enable & list. \n");
	for(ch=0 ; ch<num ; ch++)
	{
		sprintf(section, "Parking%d", ch+1 );
		ret = GetPrivateProfileString( section, "Enable", "1", tmp, 32, INI );
		ret = ((u32)(ntohl(inet_addr(tmp))));
		
		if(ret == 0)
		{
			vsc_lpr[ch].ch_id = -1;
			sprintf(log_msg,"[MAIN] [Init] [ch%04d] id = %d. \n", ch+1, vsc_lpr[ch].ch_id );
			Write_Log(0,log_msg);
		}
		else
		{
			vsc_lpr[ch].ch_id = ch;
			sprintf(log_msg,"[MAIN] [Init] [ch%04d] id = %d. \n", ch+1, vsc_lpr[ch].ch_id );
			Write_Log(0,log_msg);
			vsc_lpr[ch].ip = 0;
			int i;
			for (i=0 ; i<4 ; i++)
			{
				vsc_lpr[ch].handle_lpr[i].parkingID = -1;
				vsc_lpr[ch].handle_lpr[i].buf_flag = vsc_lpr[ch].handle_lpr[i].lost_event_times = 0;
			}
			

		}
	}	

	system("echo get vsc list > ./boot.log");

	total_ch = 0;
	for(ch=0 ; ch<num ; ch++)
	{
		if(vsc_lpr[ch].ch_id != -1)
			get_vsc_list( ch, vsc_lpr, "3531.ini");
	}

	system("echo ntp time check > ./boot.log");

	//確認時間有正常更新
	int			dbg_enable;
	ret = GetPrivateProfileString( "System", "DBG", "0", tmp, 32, INI );
	dbg_enable = atoi(tmp);

	struct tm event_time;
	time_t stime;
	struct timeval tt;
	u32 now;

	gettimeofday(&tt, 0);
	now = (u32) tt.tv_sec;
	if(!dbg_enable)
	{
		event_time.tm_year = 2014-1900;
		event_time.tm_mon = 0;
		event_time.tm_mday = 1;
		event_time.tm_hour = 0;
		event_time.tm_min = 0;
		event_time.tm_sec = 0;

		stime = mktime(&event_time);

		Write_Log(0,"[MAIN] [Init] Check ntp time. \n");
		//printf("[Init] Check ntp time. \n");
		while(now < stime)
		{
			gettimeofday(&tt, 0);
			now = (u32) tt.tv_sec;
			mcu_wdt_reset(WDT_MARGIN_TIME);
			sleep(2);
		}
	}
	system("echo create socket server > ./boot.log");

	mcu_wdt_reset(WDT_MARGIN_TIME);
	sprintf(log_msg,"[MAIN] [Init] [ch%04d] Init ipc total_ch=%d. \n", ch+1, total_ch);
	Write_Log(0,log_msg);
	for(ch=0 ; ch<num ; ch++)
	{
		sprintf(log_msg,"[MAIN] [Init] [ch%04d] before socket --- ip =%d. \n", ch+1, vsc_lpr[ch].ip);
		Write_Log(0,log_msg);
	}

	strcpy(rc.path, "/mnt/data");

	//設定錄影參數
	rc.ch_num = total_ch;
	for(ch=0 ; ch<total_ch ; ch++)
	{
		rc.ch_info[ch].ip = vsc_lpr[ch].ip;
		rc.ch_info[ch].port = vsc_lpr[ch].port;
		rc.ch_info[ch].ch = 0;
		rc.ch_info[ch].id = 0;
		rc.ch_info[ch].fps = rec_fps;
		rc.ch_info[ch].rec->heigh = 0;
		rc.ch_info[ch].rec->width = 0;
	}

	socket_ipc = InitSocket(8201, sock_callback, 0 , &rc);
	if(socket_ipc == 0)
		Write_Log(0,"[MAIN] [Init] [socket_ERROR] ipc create fail. \n");
		//printf("[socket_ERROR] ipc create fail. \n");
	else
	{
		//printf("[IPC] set ipc channel list. \n");
		Write_Log(0,"[MAIN] [Init] set ipc channel list. \n");
		for (ch=0 ; ch<total_ch ; ch++)
		{
			if(vsc_lpr[ch].handle_lpr[0].parkingID != -1)
			{
				if(vsc_lpr[ch].handle_lpr[1].parkingID == -1){
					Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[0].parkingID, 0);//0 => C
					Insert_rec_list(ch, vsc_lpr[ch].handle_lpr[0].parkingID);//0 => C
				}
				else {
					Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[0].parkingID, 1);//1 => L
					Insert_rec_list(ch, vsc_lpr[ch].handle_lpr[0].parkingID);//1 => L
				}
				park_list[vsc_lpr[ch].handle_lpr[0].parkingID-1] = ch+1;
			}
			if(vsc_lpr[ch].handle_lpr[1].parkingID != -1){
				Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[1].parkingID, 2);// 2 => R
				Insert_rec_list(ch, vsc_lpr[ch].handle_lpr[1].parkingID);// 2 => R
				park_list[vsc_lpr[ch].handle_lpr[1].parkingID-1] = ch+1;

			}

			if(vsc_lpr[ch].handle_lpr[2].parkingID != -1)
			{
				if(vsc_lpr[ch].handle_lpr[3].parkingID == -1){
					Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[2].parkingID, 0);//0 => C
					Insert_rec_list(ch, vsc_lpr[ch].handle_lpr[2].parkingID);//0 => C
				}
				else {
					Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[2].parkingID, 1);//1 => L
					Insert_rec_list(ch, vsc_lpr[ch].handle_lpr[2].parkingID);//1 => L
				}
				park_list[vsc_lpr[ch].handle_lpr[2].parkingID-1] = ch+1;

			}
			if(vsc_lpr[ch].handle_lpr[3].parkingID != -1){
				Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[3].parkingID, 2);// 2 => R
				Insert_rec_list(ch, vsc_lpr[ch].handle_lpr[3].parkingID);// 2 => R
				park_list[vsc_lpr[ch].handle_lpr[3].parkingID-1] = ch+1;

			}
		}

		if( sock_start_listening(socket_ipc) == -1)
			Write_Log(0,"[MAIN] [Init] [socket_ERROR] ipc listen fail. \n");
			//printf("[socket_ERROR] ipc listen fail. \n");
		else
			Write_Log(0,"[MAIN] [Init] [socket] ipc listen success. \n");
			//printf("[socket] ipc listen success. \n");
	}

	system("echo recorder init > ./boot.log");
	Write_Log(0,"[MAIN] [Init] init codec. \n");
	//printf("[INIT] init codec. \n");
	//錄影
	mcu_wdt_reset(WDT_MARGIN_TIME);

	/*/確認時間有正常更新
	int			dbg_enable;
	ret = GetPrivateProfileString( "System", "DBG", "0", tmp, 32, INI );
	dbg_enable = atoi(tmp);

	struct tm event_time;
	time_t stime;
	struct timeval tt;
	u32 now;

	gettimeofday(&tt, 0);
	now = (u32) tt.tv_sec;
	if(!dbg_enable)
	{
		event_time.tm_year = 2014-1900;
		event_time.tm_mon = 0;
		event_time.tm_mday = 1;
		event_time.tm_hour = 0;
		event_time.tm_min = 0;
		event_time.tm_sec = 0;

		stime = mktime(&event_time);
		
		printf("[Init] Check ntp time. \n");
		while(now < stime)
		{
			gettimeofday(&tt, 0);
			now = (u32) tt.tv_sec;
			sleep(2);
		}
	}*/
	
	//stime = now;
	//printf("[EVENT] now=%d, time_t=%d. ,%04d//%02d//%02d - %02d:%02d%:02d,\n", now, time, event_time.tm_year+1900, event_time.tm_mon+1, event_time.tm_mday, event_time.tm_hour, event_time.tm_min, event_time.tm_sec);
	//開始錄影
	codec_init();
	for(ch=0 ; ch<num ; ch++)
	{
		sprintf(log_msg,"[MAIN] [Init] [ch%04d]  before rec --- ip =%d. \n", ch+1, vsc_lpr[ch].ip);
		Write_Log(0,log_msg);
		sprintf(log_msg,"[MAIN] [Motion] %d\n",park_list[ch]);
		Write_Log(0,log_msg);

	}

	mcu_wdt_reset(WDT_MARGIN_TIME);
	
	rec_init(&rc);

	system("echo socket client init > ./boot.log");
	ret = GetPrivateProfileString( "System", "loc_IP", "0", loc_ip, 32, INI );
	ret = GetPrivateProfileString( "System", "GW_Server", "0", gw_ip, 32, INI );
	sprintf(log_msg,"[MAIN] [Init] loc ip =%s, GW ip =%s. \n", loc_ip, gw_ip);
	Write_Log(0,log_msg);
	
	mcu_wdt_reset(WDT_MARGIN_TIME);
	hgc = Create_client(gw_ip, port, loc_ip);
	//vsc_lpr init
	for(ch=0 ; ch<num ; ch++)
	{
		mcu_wdt_reset(WDT_MARGIN_TIME);
		sprintf(log_msg,"[MAIN] [Init] [ch%04d] into ip =%d. \n", vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].ip);
		Write_Log(0,log_msg);
		if( vsc_lpr[ch].ip != 0 )
		{
			for(x=0 ; x<4 ; x++)
			{
				//sprintf(tmp, "echo init ch%d, > ./t.log", vsc_lpr[ch].handle_lpr[x].parkingID);
				//system(tmp);

				//初始事件紀錄
				sprintf(log_msg,"[MAIN] [Init] [ch%04d] [EVENT] tag=0 set empty event time=%d. \n", vsc_lpr[ch].handle_lpr[x].parkingID, now);
				Write_Log(0,log_msg);
				rec_event_plate(vsc_lpr[ch].ch_id, vsc_lpr[ch].handle_lpr[x].parkingID, 0, now, (u32)tt.tv_usec, 0, 0, 0, 0);
				vsc_lpr[ch].handle_lpr[0].state = 0;
				vsc_lpr[ch].handle_lpr[0].max_lpr_counter = 0;
				vsc_lpr[ch].handle_lpr[0].no_ans = 0;

				Clean_Answer_List(vsc_lpr[ch].handle_lpr[x].parkingID, now, (u32)tt.tv_usec);

				sprintf(log_msg,"[MAIN] [Init] [ch%04d] padding[%d]. \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
				Write_Log(0,log_msg);
				if(vsc_lpr[ch].handle_lpr[x].parkingID != -1)
				{
					/*rc.ch_info[vsc_lpr[ch].handle_lpr[x].parkingID-1].rec->vsc_id = ch;
					sprintf(log_msg,"[INS] [TEST] [vs%04d] padding[%d] = ch%04d , VS = %d. \n", ch+1, x, vsc_lpr[ch].handle_lpr[x].parkingID,rc.ch_info[vsc_lpr[ch].handle_lpr[x].parkingID-1].rec->vsc_id);
					Write_Log(0,log_msg);*/
					sprintf(log_msg,"[MAIN] [Init] [ch%04d] - alloc lpr[%d] buffer \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
					Write_Log(0,log_msg);
					vsc_lpr[ch].handle_lpr[x].image_buf = (u8*)malloc(BSIZE);
					if(vsc_lpr[ch].handle_lpr[x].image_buf == NULL)
					{
						sprintf(log_msg,"[MAIN] [Init] [ch%04d] [ERROR] - lpr[%d] malloc parking image buffer fail \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
						Write_Log(0,log_msg);
					}
					sprintf(log_msg,"[MAIN] [Init] [ch%04d] - alloc lpr[%d] handle \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
					Write_Log(0,log_msg);
					vsc_lpr[ch].handle_lpr[x].lpr = lpr_alloc(vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].handle_lpr[x].result_list, "3531.ini");
					if(!vsc_lpr[ch].handle_lpr[x].lpr) 
					{
						sprintf(log_msg,"[MAIN] [Init] [ch%04d] [ERROR] - lpr[%d] alloc lpr fail \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
						Write_Log(0,log_msg);
					}

					sprintf(log_msg,"[MAIN] [Init] [ch%04d]- set lpr[%d] param \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
					Write_Log(0,log_msg);
					pthread_mutex_init(&(vsc_lpr[ch].handle_lpr[x].thread_mutex),NULL);

					sprintf(log_msg,"[MAIN] [Init] [ch%04d] - init lpr[%d] semaphore \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
					Write_Log(0,log_msg);
					if ( sem_init(&(vsc_lpr[ch].handle_lpr[x].case_evnet), 1, 0) != 0)
					{
						sprintf(log_msg,"[MAIN] [Init] [ch%04d] [ERROR] - lpr[%d] Semaphore initialization failed \n", vsc_lpr[ch].handle_lpr[x].parkingID, x);
						Write_Log(0,log_msg);
					}

					vsc_lpr[ch].handle_lpr[x].state = -1;
					int y = 0;
					for (y=0 ; y<MAX_CH ; y++)
					{
						vsc_lpr[ch].handle_lpr[x].result_list[y].cout = 0;
						sprintf(vsc_lpr[ch].handle_lpr[x].result_list[y].plate, "0000");
					}
					
					sprintf(section, "Parking%d", vsc_lpr[ch].handle_lpr[x].parkingID );
					sprintf(log_msg,"[MAIN] [Init] [ch%04d] read %s top_base. \n", vsc_lpr[ch].handle_lpr[x].parkingID, section);
					Write_Log(0,log_msg);
					//base param
					ret = GetPrivateProfileString( "System", "Top_No", "100", tmp, 32, INI );
					vsc_lpr[ch].handle_lpr[x].top_cout = atoi(tmp);
					if (vsc_lpr[ch].handle_lpr[x].top_cout == 0)
						vsc_lpr[ch].handle_lpr[x].top_cout = 100;			

					ret = GetPrivateProfileString( "System", "Base_No", "5", tmp, 32, INI );
					vsc_lpr[ch].handle_lpr[x].base_cout = atoi(tmp);
					if (vsc_lpr[ch].handle_lpr[x].base_cout == 0)
						vsc_lpr[ch].handle_lpr[x].base_cout = 5;			

					ret = GetPrivateProfileString( "System", "Open_Flag", "1", tmp, 32, INI );
					vsc_lpr[ch].handle_lpr[x].open_flag = atoi(tmp);

					sprintf(log_msg,"[MAIN] [Init] [ch%04d] top=%d, base=%d Vs_port = %d. \n", vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].handle_lpr->top_cout, vsc_lpr[ch].handle_lpr->base_cout,vsc_lpr[ch].port);
					Write_Log(0,log_msg);
					
					Set_sck_param(hgc, vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].handle_lpr[x].base_cout, vsc_lpr[ch].handle_lpr[x].top_cout, vsc_lpr[ch].handle_lpr[x].result_list, &(vsc_lpr[ch].handle_lpr[x].ev_empty), &vsc_lpr[ch].handle_lpr[x].thread_mutex);

					vsc_lpr[ch].handle_lpr[x].ch_id = vsc_lpr[ch].ch_id;
					
					sprintf(log_msg,"[MAIN] [Init] [ch%04d] create thread. \n", vsc_lpr[ch].handle_lpr[x].parkingID);
					Write_Log(0,log_msg);
					//sprintf(log_msg,"[TEST][vs%04d] - vsc_id %d \n", ch+1 , rc.ch_info[vsc_lpr[ch].handle_lpr[x].parkingID-1].rec->vsc_id);
					//Write_Log(0,log_msg);
					create_thread(lpr_test, &(vsc_lpr[ch].handle_lpr[x]));		
				}
			}

// 			if(vsc_lpr[ch].parkingID[1] != -1)
// 			{
// 				printf("[ch%d] [vs%d] create lpr[1] thread. \n", ch+1, vsc_lpr[ch].parkingID[1]);
// 				create_thread(lpr_test1, &vsc_lpr[ch]);
// 			}
			sprintf(log_msg,"[MAIN] [Init] [ch%04d] - alloc vsc handle ID port =%d \n", vsc_lpr[ch].handle_lpr[x].parkingID,vsc_lpr[ch].port);
			Write_Log(0,log_msg);
			vsc_lpr[ch].vsc = vsc_alloc(vsc_lpr[ch].ip, ch+1, 0, 1,vsc_lpr[ch].port, vsc_callback, &vsc_lpr[ch]);	//車位偵測的stream
		}
		else{
			sprintf(log_msg,"[MAIN] [Init] [ch%04d] ip %d not working \n", vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].ip);
			Write_Log(0,log_msg);}
	}
	system("echo socket client init over > ./boot.log");

	//printf("[GWC] Create hgc_thread (1) \n");
	Write_Log(0,"[MAIN] [Init] Create hgc_thread (1) \n");
	mcu_wdt_reset(WDT_MARGIN_TIME);
	Gateway_Connect(hgc);



	//mcu function
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);

	//create_thread(clean_dbg, &("/dbg"));

	//mcu_init(mcu_callback);

	/*mcu_wdt_reset(WDT_MARGIN_TIME);
	void *tttt;
	//printf("[TEST] [MUC] %s - mcu test", vsc_lpr[0].handle_lpr[0].parkingID);
	//RAW2File("/aaa/bbb.obi",tttt, 111,222,8,IMG_FMT_BITMAP,0);
	tttt = (char *)malloc(32*sizeof(char));
	free(tttt);
	free(tttt);*/
	system("echo main over > ./boot.log");

	//Vs_state = new int [num+1];
	////Ask_Vs_State(Vs_state);
	//if (Vs_state == 0)
	//{
	//	Write_Log(0,"[MAIN] Vs_state == 0 !\n");
	//}

	while(1) 
	{
		if(mcu_wdt_reset(WDT_MARGIN_TIME) < 0)
		{Write_Log(0,"[MAIN] mcu_wdt_reset fail\n");}
		sleep(5);
		V_F_count++;
		R_count++;
		if (V_F_count == 35)
		{
			Ask_Vs_State(&Vs_state);
			Write_Log(3,"[MAIN] Change Log!\n");
			//int z;
			//for (z = 0;z<num+1 ; z++)
			//{
			//	sprintf(log_msg,"[MAIN_T] Ask_Vs_State = %d\n", Vs_state[z]);
			//	Write_Log(0,log_msg);
			//}	
			Set_Vsfail(Vs_state,num);
			V_F_count = 0;
		}
		//if (R_count==719)
		//{
		//	system("reboot");
		//}
		
	}

	return 0;

	/*
	在libOBSysInf.a我有porting以下原Window相容的function. 您可直接使用

	int GetPrivateProfileInt( char *section, char *entry, int def, char *file_name );  //not work
	int GetPrivateProfileString( char *section, char *entry, char *def, char *buffer, int buffer_len, char *file_name );
	int WritePrivateProfileString( char *section, char *entry, char *buffer, char *file_name );
	*/
}

