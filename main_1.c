#include "ob_platform.h"
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
#define SHEET       2
#define ASIZE		5

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

	as_yframe_t			fh[ASIZE];

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

	u32					timestamp[ASIZE];
	u32					micro_sec[ASIZE];

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
	//int					parkingID[2];

	void				*vsc;
	lpr_info			handle_lpr[2];

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

static int				wflag = 0;
static int				rflag = 0;
static int				asize = ASIZE;


// -------------------------------------------------------------------------------------
static void quit_signal(int sig)
{
	//printf("[QUIT] get quit signal");
	Write_Log(0,"[QUIT] get quit signal");
 	if (mcu_wdt_disable() < 0) {
 		dbg("wdt_disable error\n");
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
	char		section[32];
	char		tmp[32];
	u32			ip;

	int			x;

	memset(section, 0 ,32);
	memset(tmp, 0 ,32);

	sprintf(log_msg,"[PARKINGLIST] ch_id=%d, filename=%s .\n", channel_id+1, file_name );
	Write_Log(0,log_msg);
	sprintf(section, "Parking%d", channel_id+1 );
	ret = GetPrivateProfileString( section, "IP_Addr", "0.0.0.0", tmp, 32, file_name );
	ip = ((u32)(ntohl(inet_addr(tmp))));

	sprintf(section, "Parking%d", channel_id+1 );
	ret = GetPrivateProfileString( section, "Parking_ID", "0", tmp, 32, file_name );
	id = atoi(tmp);

	if(id==0){id=channel_id+1;}

	sprintf(section, "Parking%d", channel_id+1 );
	ret = GetPrivateProfileString( section, "ROI_Index", "0", tmp, 32, file_name );
	roi = atoi(tmp);

	sprintf(log_msg,"[PARKINGLIST] get list %d times, ip=%d, park=%d, roi=%d.\n", channel_id+1,  ip, id, roi);
	Write_Log(1,log_msg);

	if(ip != 0)
	{
		sprintf(log_msg,"[PARKINGLIST] get into find list \n");
		Write_Log(1,log_msg);
		for(x=0 ; x<32 ; x++ )
		{
		
			sprintf(log_msg,"[PARKINGLIST] find list[%d] ip=%d\n", x, ch_info[x].ip);
			Write_Log(1,log_msg);
			if (ch_info[x].ip == ip)
			{
				sprintf(log_msg,"[PARKINGLIST] ip=%d find in list[%d] \n", ip ,x );
				Write_Log(0,log_msg);
				if( ch_info[x].handle_lpr[roi-1].parkingID != -1 )
				{//已有設定過
					sprintf(log_msg,"[ERROR]roi_index[%d] is used \n", roi );
					Write_Log(0,log_msg);
					return 0-channel_id;
				}
				else
				{
					//ch_info[x].handle_lpr[roi-1].ch_id = 
					ch_info[x].handle_lpr[roi-1].parkingID = id;
					sprintf(log_msg,"[PARKINGLIST] list[%d] roi_index[%d] set %d \n", x, roi, ch_info[x].handle_lpr[roi-1].parkingID );
					Write_Log(1,log_msg);
					break;
				}
			}
			else if(ch_info[x].ip == 0)
			{//ip not find
				ch_info[x].ip = ip;
				ch_info[x].handle_lpr[roi-1].parkingID = id;

				sprintf(log_msg,"[PARKINGLIST] add in list[%d] ip=%d  ,roi=%d, park=%d. \n", x, ip, roi, ch_info[x].handle_lpr[roi-1].parkingID);
				Write_Log(0,log_msg);
				total_ch = total_ch + 1;
				break;
			}	
		}
	}
	else
	{
		sprintf(log_msg,"not use ch%04d \n", channel_id+1);
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

	sprintf(log_msg,"[callback] vsc%d get padding%d callback size=%d \n", wPriv->ch_id+1, wfh->id, wfh->size);
	Write_Log(1,log_msg);
	sprintf(log_msg,"[callback] vsc%d -- lpr parkID[%d]=%d. \n", wPriv->ch_id+1, wfh->id, wPriv->handle_lpr[wfh->id].parkingID );
	Write_Log(1,log_msg);
    //dbg("frame in\n");
	if(wPriv->handle_lpr[wfh->id].parkingID != -1)
	{
		if (wfh->size != 0)
		{
			sleep(3);
			sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] get event ,tag=%d size=%d \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1, wfh->id, wfh->size);
			Write_Log(1,log_msg);
			wPriv->handle_lpr[wfh->id].state = 1;

			//pthread_mutex_lock(&(wPriv->handle_lpr[wfh->id].thread_mutex));
			//if (wPriv->handle_lpr[wfh->id].buf_flag == 0)
			if(wflag+1 > rflag)
			{
				//wPriv->handle_lpr[wfh->id].buf_flag = 1;

				sprintf(log_msg,"[callback] [ch%04d] wflag = %d rflag = %d\n",wPriv->handle_lpr[wfh->id].parkingID,wflag,rflag);
				Write_Log(1,log_msg);

				//複製資料到辨識的參數
				memcpy(&(wPriv->handle_lpr[wfh->id].fh[wflag]), fh, sizeof(as_yframe_t));
				memcpy( wPriv->handle_lpr[wfh->id].image_buf+(wflag*(wfh->height*wfh->width*SHEET)), data, wfh->size);

				wPriv->handle_lpr[wfh->id].timestamp[wflag] = now;
				wPriv->handle_lpr[wfh->id].micro_sec[wflag] = (u32)tt.tv_usec;
				wflag =(wflag + 1)%(asize+1);
				//if(wflag==6){wflag=0;rflag=1;}
				sprintf(log_msg,"[callback] [ch%04d] [CPY] wflag = %d rflag = %d\n",wPriv->handle_lpr[wfh->id].parkingID,wflag,rflag);
				Write_Log(1,log_msg);
				sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] get event, now=%d, usec=%d \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1, wPriv->handle_lpr[wfh->id].timestamp, wPriv->handle_lpr[wfh->id].micro_sec);
				Write_Log(1,log_msg);
				//wPriv->handle_lpr[wfh->id].buf_flag = 0;

				sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] - send lpr event \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1 );
				Write_Log(0,log_msg);
				sem_post( &(wPriv->handle_lpr[wfh->id].case_evnet) );
			}
			else if(wflag+1 <= rflag)
			{
				sem_post( &(wPriv->handle_lpr[wfh->id].case_evnet) );
				wPriv->handle_lpr[wfh->id].lost_event_times++;
				sprintf(log_msg,"[callback] [ch%04d] [FULL] wflag = %d rflag = %d\n",wPriv->handle_lpr[wfh->id].parkingID,wflag,rflag);
				Write_Log(1,log_msg);
				sprintf(log_msg,"[WARNING][ch%04d] callback-vsc is full \n", wPriv->handle_lpr[wfh->id].parkingID);
				Write_Log(0,log_msg);
				sprintf(log_msg,"[WARNING][ch%04d] callback-vsc[%d] - lost times = %d \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1, wPriv->handle_lpr[wfh->id].lost_event_times);
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
					wPriv->handle_lpr[wfh->id].state = 0;
					sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] insert - 0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
					Write_Log(1,log_msg);
					rec_event_plate(wPriv->ch_id, wPriv->handle_lpr[wfh->id].parkingID, 0, now, (u32)tt.tv_usec, 0, 0, 0, 0);

					//pthread_mutex_lock(&(wPriv->handle_lpr[wfh->id].thread_mutex));
					sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] clean lpr list - 0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
					Write_Log(1,log_msg);
					wPriv->handle_lpr[wfh->id].max_lpr_counter = 0;
					wPriv->handle_lpr[wfh->id].no_ans = 0;
					wPriv->handle_lpr[wfh->id].ev_empty.timestamp = now;
					wPriv->handle_lpr[wfh->id].ev_empty.micro_sec = (u32)tt.tv_usec;
					
					lpr_list_clean(wPriv->handle_lpr[wfh->id].lpr, wPriv->handle_lpr[wfh->id].parkingID, 0);
					Clean_Answer_List(wPriv->handle_lpr[wfh->id].parkingID, now, (u32)tt.tv_usec);
					//pthread_mutex_unlock(&(wPriv->handle_lpr[wfh->id].thread_mutex));
					sprintf(log_msg,"[callback] [ch%04d] callback-send empty. \n", wPriv->handle_lpr[wfh->id].parkingID);
					Write_Log(0,log_msg);
					//Send_List(hgc, wPriv->parkingID[wfh->id], 0, 0, 0, 0, loc_ip, 1);
					Set_flag(hgc, wPriv->handle_lpr[wfh->id].parkingID);

					sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] clean over - 0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
					Write_Log(1,log_msg);
					//InsertAnswer(socket_ipc, wPriv->parkingID[wfh->id], "0000", data);
				}
				else
					{
						sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] state=0000. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
						Write_Log(1,log_msg);
					}
					
				wPriv->handle_lpr[wfh->id].buf_flag = 0;
			}
			else
				{
					sprintf(log_msg,"[callback] [ch%04d] callback-vsc[%d] set state=0000 fail. \n", wPriv->handle_lpr[wfh->id].parkingID, wPriv->ch_id+1);
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
	int				temp_size;
    int				ret = 0;
	int				re_use = 0;
	clock_t			start, finish;

	int				sort_list[MAX_CH];
	char			tmp_plate[32];
	BYTE			tmp_buf[76800];
	
	int				send_ret = 0;
	u16				xx,yy;

	//struct timeval tt;
	//gettimeofday(&tt, 0);
	//u32 now = (u32) tt.tv_sec;

	sprintf(log_msg,"[ch%04d] start lpr function \n", wParam->parkingID);
	Write_Log(0,log_msg);
	sprintf(tmp_plate, "0000");

	while(1)
	{
		if(wParam->max_lpr_counter == 0){sprintf(tmp_plate, "0000");}
		sprintf(log_msg,"[ch%04d] wait vsc callback event \n", wParam->parkingID);
		Write_Log(1,log_msg);
		//lpr_list_clean(wParam->handle_lpr[0].lpr);
		sem_wait( &(wParam->case_evnet) );

		if(wParam->max_lpr_counter >= wParam->top_cout)	//某一答案達到上限設定值
		{
			if(wParam->open_flag == 1)
			{	
				//Set_flag(hgc, wParam->parkingID);
				lpr_list_clean(wParam->lpr, wParam->parkingID, 1);	
				wParam->max_lpr_counter = 0;
				sprintf(tmp_plate, "0000");
			}
			else
			{
				sprintf(log_msg,"[ch%04d] max(%d) is enough(%d) ,stop lpr \n", wParam->parkingID, wParam->max_lpr_counter, wParam->top_cout);
				Write_Log(0,log_msg);
				//sleep(1);
				continue;
			}
		}

		frame_num = wParam->fh[rflag].size / (wParam->fh[rflag].width*wParam->fh[rflag].height);
		sprintf(log_msg,"[ch%04d] get vsc callback event ,fh size=%d ,frame num=%d \n", wParam->parkingID, wParam->fh[rflag].size, frame_num);
		Write_Log(0,log_msg);
		sprintf(log_msg,"[LPRTEST] [ch%04d] [OUTWRITE] wflag=%d , rflag=%d \n", wParam->parkingID, wflag, rflag);
		Write_Log(0,log_msg);
		if( (rflag+1)%(asize+1) == wflag)
		{
			sprintf(log_msg,"[ch%04d] try to lock img-buffer fail \n", wParam->parkingID);
			Write_Log(0,log_msg);
			sprintf(log_msg,"[LPRTEST] [ch%04d] [EMPTY] wflag=%d , rflag=%d \n", wParam->parkingID, wflag, rflag);
			Write_Log(0,log_msg);
			rflag = 0;
		}
		else if(wflag < rflag)
		{
			sprintf(log_msg,"[LPRTEST] [ch%04d] [WRITE] wflag=%d , rflag=%d \n", wParam->parkingID, wflag, rflag);
			Write_Log(0,log_msg);
			//wParam->buf_flag = 1;
			//pthread_mutex_lock(&(wParam->thread_mutex));
			//if(rflag==6){rflag=0;wflag=1;}
			temp_size = rflag*wParam->fh[rflag].width*wParam->fh[rflag].height;
			for (re_use=0 ; re_use<frame_num ; re_use++)
			{
				start = clock();
				sprintf(log_msg,"[ch%04d] send frame%d into lpr (%d) - buf(%X) \n", wParam->parkingID , re_use+1, wParam->lpr, wParam->image_buf+(wParam->fh[rflag].width*wParam->fh[rflag].height*re_use));
				Write_Log(1,log_msg);
				//sprintf(path, "//mnt//dbg//test%d.obi", wParam->parkingID[0]);
				//RAW2File(path, wParam->handle_lpr[0].image_buf+(wParam->handle_lpr[0].fh.width*wParam->handle_lpr[0].fh.height*re_use), 320, 240, 8, IMG_FMT_OBI, 0);
				ret = lpr_process(wParam->lpr, (wParam->image_buf+temp_size)+(wParam->fh[rflag].width*wParam->fh[rflag].height*re_use), wParam->timestamp[rflag], wParam->micro_sec[rflag]);
				
			}
			//pthread_mutex_unlock(&(wParam->thread_mutex));

			finish = clock();

			if(ret == 0) 
			{
				u32 elapsed = (u32)(finish - start) * 1000 / CLOCKS_PER_SEC;
			
				sprintf(log_msg,"[ch%04d] LPR takes %u msec\n", wParam->parkingID ,elapsed);
				Write_Log(1,log_msg);

				ret = lpr_dump_result(wParam->lpr, &(wParam->voting_ans), tmp_buf);

				if(ret > 0)
				{
					sprintf(log_msg,"[ch%04d] dump result is %s , ret = %d. \n", wParam->parkingID, wParam->voting_ans.plate, ret);
					Write_Log(0,log_msg);
					//printf("[XXX] 001 (ch%d)-%ld. \n", wParam->parkingID,wParam->result_list);
					xx = wParam->fh[rflag].x;
					yy = wParam->fh[rflag].y;

					if( (strcmp(tmp_plate, wParam->voting_ans.plate)!=0)) //答案不重複
					{
						rec_event_plate(wParam->ch_id, wParam->parkingID, wParam->voting_ans.plate, wParam->voting_ans.timestamp, wParam->voting_ans.micro_sec, xx+wParam->voting_ans.x1, yy+wParam->voting_ans.y1, wParam->voting_ans.x2-wParam->voting_ans.x1, wParam->voting_ans.y2-wParam->voting_ans.y1);
					
						//Insert_Answer_List(wParam->parkingID[0], wParam->handle_lpr[0].voting_ans.plate, wParam->handle_lpr[0].voting_ans.timestamp, wParam->handle_lpr[0].voting_ans.micro_sec);

						//memset(sort_list, 0 ,MAX_CH);
						//lpr_list_sort(wParam->parkingID[0], sort_list, wParam->handle_lpr[0].result_list);

						Set_flag(hgc, wParam->parkingID);
						//Send_List(hgc, wParam->parkingID[0], wParam->handle_lpr[0].base_cout, sort_list, (void *)wParam->handlse_lpr[0].result_list, &(wParam->handle_lpr[0].ev_empty), loc_ip, 0);

						sprintf(log_msg,"[ch%04d] Send ret=%d", wParam->parkingID, ret);
						Write_Log(0,log_msg);

						strcpy(tmp_plate, wParam->voting_ans.plate);
					}
					wParam->max_lpr_counter = ret;

					if (ret >= wParam->top_cout)
					{
						Set_flag(hgc, wParam->parkingID);
					}
					
				}
				else if(ret == 0)//##-####
				{
					sprintf(log_msg,"[ch%04d] dump result is ##-#### , ret = %d. \n", wParam->parkingID, ret);
					Write_Log(0,log_msg);
				}
				else
				{
					sprintf(log_msg,"[ch%04d][ERROR] lpr dump result error\n", wParam->parkingID);
					Write_Log(0,log_msg);
				}
				sprintf(log_msg,"[ch%04d] tmp_plate is %s , conter = %d. \n", wParam->parkingID, tmp_plate, wParam->max_lpr_counter);
				Write_Log(0,log_msg);
			}
			else 
			{
				sprintf(log_msg,"[ch%04d]LPR fail. \n", wParam->parkingID);
				Write_Log(0,log_msg);
			}
			rflag = (rflag + 1)%asize+1;
			wParam->buf_flag = 0;
			sprintf(log_msg,"[ch%04d] lpr event over. \n", wParam->parkingID);
			Write_Log(0,log_msg);
		}
	}
	
exit_lpr_test:

	sprintf(log_msg,"[MAIN] [FREE] ch%04d free lpr ", wParam->parkingID);
	Write_Log(1,log_msg);
    lpr_free(wParam->lpr);


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
	//VERSION 1 目前出版							                       //
	//										 							   //
	//                                                                     //
	//                                                                     //
	//                                                                     //
	/////////////////////////////////////////////////////////////////////////
	if(mcu_init(mcu_callback) < 0)
		Write_Log(0,"[MCU] init fail\n");
	//printf("[MCU] init fail");
	int muc_cout = 0;
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
	int					ch, ret, x,re,mkd;
	char				section[16], tmp[64], time_path[128];
	while(1)
	{
		struct timeval tt;
		struct	tm *now;
		char	time_buffer_yes[128];
		char	time_buffer_now[128];
		char	time_year[128];
		int		year;

		ret = GetPrivateProfileString( "System", "GW_Server", "0", gw_ip, 32, INI );

		gettimeofday(&tt, NULL);
		now = localtime(&tt.tv_sec);
		strftime (time_buffer_yes, 80, "%Y%m%d%H%M%S", now);
		sprintf(time_path,"./ntpclient -s -t -i 10 -h %s",gw_ip);
		system(time_path);
		gettimeofday(&tt, NULL);
		now = localtime(&tt.tv_sec);
		strftime (time_buffer_now, 80, "%Y%m%d%H%M%S", now);
		strftime (time_year, 80, "%Y", now);
		year = atoi(time_year);
		if(year>=2011 && year<=2017){break;}
		system("echo ntp get time > ./boot.log");
	}
	system("echo start main obnvr>./boot.log");
	Write_Log(0,"==================OBNVR VERSION 3==================\n");
//CREATE DIR============================================================================
	if((re=remove("/dbg/lpr"))!=0)
	{
		Write_Log(0,"remove dbg/lpr fail\n");
		//printf("remove dbg/lpr fail\n");
	}
	if((mkd=mkdir("/dbg/lpr",S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
	{
		Write_Log(0,"mkdir dbg/lpr fail\n");
		system("echo mkdir dbg/lpr fail>./boot.log");
		//printf("mkdir dbg/lpr fail\n");
	}

	/*if((re=remove("/dbg/log"))!=0)
	{
		Write_Log(0,"remove dbg/log fail\n");
		//printf("remove dbg/log fail\n");
	}
	if((mkd=mkdir("/dbg/log",S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
	{
		Write_Log(0,"mkdir dbg/log fail\n");
		//printf("mkdir dbg/log fail\n");
	}

	if((re=remove("/dbg/vs"))!=0)
	{
		Write_Log(0,"remove dbg/vs fail\n");
		//printf("remove dbg/vs fail\n");
	}
	if((mkd=mkdir("/dbg/vs",S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
	{
		Write_Log(0,"mkdir dbg/vs fail\n");
		//printf("mkdir dbg/vs fail\n");
	}*/

//=======================================================================================

	int					num = 0;
	ret = GetPrivateProfileString( "System", "TotalNum", "32", tmp, 32, INI );
	num = ((u32)(ntohl(inet_addr(tmp))));
	sprintf(log_msg,"[Init] get total num = %d. \n", num);
	Write_Log(0,log_msg);
	//printf("[Init] get total num = %d. \n", num);

	//printf( "init vsc_lpr enable & list. \n" );
	Write_Log(0,"init vsc_lpr enable & list. \n");
	for(ch=0 ; ch<num ; ch++)
	{
		sprintf(section, "Parking%d", ch+1 );
		ret = GetPrivateProfileString( section, "Enable", "1", tmp, 32, INI );
		ret = ((u32)(ntohl(inet_addr(tmp))));
		
		if(ret == 0)
		{
			vsc_lpr[ch].ch_id = -1;
			sprintf(log_msg,"[Init] ch%04d id = %d. \n", ch+1, vsc_lpr[ch].ch_id );
			Write_Log(0,log_msg);
		}
		else
		{
			vsc_lpr[ch].ch_id = ch;
			sprintf(log_msg,"[Init] ch%04d id = %d. \n", ch+1, vsc_lpr[ch].ch_id );
			Write_Log(0,log_msg);
			vsc_lpr[ch].ip = 0;
			vsc_lpr[ch].handle_lpr[0].parkingID = vsc_lpr[ch].handle_lpr[1].parkingID = -1;
			vsc_lpr[ch].handle_lpr[0].buf_flag = vsc_lpr[ch].handle_lpr[0].lost_event_times = 0;
			vsc_lpr[ch].handle_lpr[1].buf_flag = vsc_lpr[ch].handle_lpr[1].lost_event_times = 0;
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

		Write_Log(0,"[Init] Check ntp time. \n");
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
	sprintf(log_msg,"[IPC] Init ipc total_ch=%d. \n", total_ch);
	Write_Log(0,log_msg);
	for(ch=0 ; ch<num ; ch++)
	{
		sprintf(log_msg,"[ch%04d] before socket --- ip =%d. \n", ch+1, vsc_lpr[ch].ip);
		Write_Log(0,log_msg);
	}

	socket_ipc = InitSocket(8201, sock_callback, 0);
	if(socket_ipc == 0)
		Write_Log(0,"[socket_ERROR] ipc create fail. \n");
		//printf("[socket_ERROR] ipc create fail. \n");
	else
	{
		//printf("[IPC] set ipc channel list. \n");
		Write_Log(0,"[IPC] set ipc channel list. \n");
		for (ch=0 ; ch<total_ch ; ch++)
		{
			if(vsc_lpr[ch].handle_lpr[0].parkingID != -1)
			{
				if(vsc_lpr[ch].handle_lpr[1].parkingID == -1)
					Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[0].parkingID, 0);
				else
					Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[0].parkingID, 1);
			}
			if(vsc_lpr[ch].handle_lpr[1].parkingID != -1)
				Insert_ch_list(ch, vsc_lpr[ch].handle_lpr[1].parkingID, 2);
		}
		
		if( sock_start_listening(socket_ipc) == -1)
			Write_Log(0,"[socket_ERROR] ipc listen fail. \n");
			//printf("[socket_ERROR] ipc listen fail. \n");
		else
			Write_Log(0,"[socket] ipc listen success. \n");
			//printf("[socket] ipc listen success. \n");
	}

	system("echo recorder init > ./boot.log");
	Write_Log(0,"[INIT] init codec. \n");
	//printf("[INIT] init codec. \n");
	//錄影
	mcu_wdt_reset(WDT_MARGIN_TIME);
	codec_init();

	strcpy(rc.path, "/mnt/data");

	//設定錄影參數
	rc.ch_num = total_ch;
	for(ch=0 ; ch<total_ch ; ch++)
	{
		rc.ch_info[ch].ip = vsc_lpr[ch].ip;
		rc.ch_info[ch].ch = 0;
		rc.ch_info[ch].id = 0;
	}

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
	for(ch=0 ; ch<num ; ch++)
	{
		sprintf(log_msg,"[vs%04d] before rec --- ip =%d. \n", ch+1, vsc_lpr[ch].ip);
		Write_Log(0,log_msg);
	}

	mcu_wdt_reset(WDT_MARGIN_TIME);
	rec_init(&rc);

	system("echo socket client init > ./boot.log");
	ret = GetPrivateProfileString( "System", "loc_IP", "0", loc_ip, 32, INI );
	ret = GetPrivateProfileString( "System", "GW_Server", "0", gw_ip, 32, INI );
	sprintf(log_msg,"[PARAM] loc ip =%s, GW ip =%s. \n", loc_ip, gw_ip);
	Write_Log(0,log_msg);
	
	mcu_wdt_reset(WDT_MARGIN_TIME);
	hgc = Create_client(gw_ip, 8201, loc_ip);
	//vsc_lpr init
	for(ch=0 ; ch<num ; ch++)
	{
		mcu_wdt_reset(WDT_MARGIN_TIME);
		sprintf(log_msg,"[INS] [vs%04d] into ip =%d. \n", ch+1, vsc_lpr[ch].ip);
		Write_Log(0,log_msg);
		if( vsc_lpr[ch].ip != 0 )
		{
			for(x=0 ; x<2 ; x++)
			{
				//sprintf(tmp, "echo init ch%d, > ./t.log", vsc_lpr[ch].handle_lpr[x].parkingID);
				//system(tmp);

				//初始事件紀錄
				sprintf(log_msg,"[INS] [vs%04d] [EVENT] ch%04d tag=0 set empty event time=%d. \n", ch+1, vsc_lpr[ch].handle_lpr[x].parkingID, now);
				Write_Log(0,log_msg);
				rec_event_plate(vsc_lpr[ch].ch_id, vsc_lpr[ch].handle_lpr[x].parkingID, 0, now, (u32)tt.tv_usec, 0, 0, 0, 0);
				vsc_lpr[ch].handle_lpr[0].state = 0;
				vsc_lpr[ch].handle_lpr[0].max_lpr_counter = 0;
				vsc_lpr[ch].handle_lpr[0].no_ans = 0;
				Clean_Answer_List(vsc_lpr[ch].handle_lpr[x].parkingID, now, (u32)tt.tv_usec);

				sprintf(log_msg,"[INS] [vs%04d] padding[%d] = ch%04d. \n", ch+1, x, vsc_lpr[ch].handle_lpr[x].parkingID);
				Write_Log(0,log_msg);
				if(vsc_lpr[ch].handle_lpr[x].parkingID != -1)
				{
					sprintf(log_msg,"[INS] [vs%04d] - alloc lpr[%d] buffer \n", ch+1, x);
					Write_Log(0,log_msg);
					vsc_lpr[ch].handle_lpr[x].image_buf = (u8*)malloc(BSIZE);
					if(vsc_lpr[ch].handle_lpr[x].image_buf == NULL){
						sprintf(log_msg,"[INS] [ERROR] [ch%04d] - lpr[%d] malloc parking image buffer fail \n", ch+1, x);
						Write_Log(0,log_msg);}
					sprintf(log_msg,"[INS] [vs%04d] - alloc lpr[%d] handle \n", ch+1, x);
					Write_Log(0,log_msg);
					vsc_lpr[ch].handle_lpr[x].lpr = lpr_alloc(vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].handle_lpr[x].result_list, "3531.ini");
					if(!vsc_lpr[ch].handle_lpr[x].lpr) 
					{
						sprintf(log_msg,"[INS] [ERROR] [ch%04d] - lpr[%d] alloc lpr fail \n", ch+1, x);
						Write_Log(0,log_msg);
					}

					sprintf(log_msg,"[INS] [vs%04d] ch%04d - set lpr[%d] param \n", ch+1, vsc_lpr[ch].handle_lpr[x].parkingID, x);
					Write_Log(0,log_msg);
					pthread_mutex_init(&(vsc_lpr[ch].handle_lpr[x].thread_mutex),NULL);

					sprintf(log_msg,"[INS] [vs%04d] ch%04d - init lpr[%d] semaphore \n", ch+1, vsc_lpr[ch].handle_lpr[x].parkingID, x);
					Write_Log(0,log_msg);
					if ( sem_init(&(vsc_lpr[ch].handle_lpr[x].case_evnet), 1, 0) != 0)
					{
						sprintf(log_msg,"[INS] [vs%04d] [ERROR] ch%04d - lpr[%d] Semaphore initialization failed \n", ch+1, vsc_lpr[ch].handle_lpr[x].parkingID, x);
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
					sprintf(log_msg,"[INS] [vs%04d] ch%04d read %s top_base. \n", ch+1, vsc_lpr[ch].handle_lpr[x].parkingID, section);
					Write_Log(0,log_msg);
					//base param
					ret = GetPrivateProfileString( section, "Top_No", "100", tmp, 32, INI );
					vsc_lpr[ch].handle_lpr[x].top_cout = atoi(tmp);
					if (vsc_lpr[ch].handle_lpr[x].top_cout == 0)
						vsc_lpr[ch].handle_lpr[x].top_cout = 100;			

					ret = GetPrivateProfileString( section, "Base_No", "5", tmp, 32, INI );
					vsc_lpr[ch].handle_lpr[x].base_cout = atoi(tmp);
					if (vsc_lpr[ch].handle_lpr[x].base_cout == 0)
						vsc_lpr[ch].handle_lpr[x].base_cout = 5;			

					ret = GetPrivateProfileString( "System", "Open_Flag", "1", tmp, 32, INI );
					vsc_lpr[ch].handle_lpr[x].open_flag = atoi(tmp);
					

					sprintf(log_msg,"[INS] [vs%04d] ch%04d top=%d, base=%d. \n", ch+1, vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].handle_lpr->top_cout, vsc_lpr[ch].handle_lpr->base_cout);
					Write_Log(0,log_msg);
					
					Set_sck_param(hgc, vsc_lpr[ch].handle_lpr[x].parkingID, vsc_lpr[ch].handle_lpr[x].base_cout, vsc_lpr[ch].handle_lpr[x].result_list, &(vsc_lpr[ch].handle_lpr[x].ev_empty), &vsc_lpr[ch].handle_lpr[x].thread_mutex);

					vsc_lpr[ch].handle_lpr[x].ch_id = vsc_lpr[ch].ch_id;
					
					sprintf(log_msg,"[INS] [vs%04d] ch%04d create thread. \n", ch+1, vsc_lpr[ch].handle_lpr[x].parkingID);
					Write_Log(0,log_msg);
					create_thread(lpr_test, &(vsc_lpr[ch].handle_lpr[x]));		
				}
			}

// 			if(vsc_lpr[ch].parkingID[1] != -1)
// 			{
// 				printf("[ch%d] [vs%d] create lpr[1] thread. \n", ch+1, vsc_lpr[ch].parkingID[1]);
// 				create_thread(lpr_test1, &vsc_lpr[ch]);
// 			}
			sprintf(log_msg,"[vs%04d] - alloc vsc handle \n", ch+1);
			Write_Log(0,log_msg);
			vsc_lpr[ch].vsc = vsc_alloc(vsc_lpr[ch].ip, 0, 0, 1, vsc_callback, &vsc_lpr[ch]);	//車位偵測的stream
		}
		else{
			sprintf(log_msg,"[vs%04d] ip %d not working \n", ch+1, vsc_lpr[ch].ip);
			Write_Log(0,log_msg);}
	}
	system("echo socket client init over > ./boot.log");

	//printf("[GWC] Create hgc_thread (1) \n");
	Write_Log(0,"[GWC] Create hgc_thread (1) \n");
	mcu_wdt_reset(WDT_MARGIN_TIME);
	Gateway_Connect(hgc);


	//mcu function
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);

	create_thread(clean_dbg, &("/dbg"));

	//mcu_init(mcu_callback);

	/*mcu_wdt_reset(WDT_MARGIN_TIME);
	void *tttt;
	//printf("[TEST] [MUC] %s - mcu test", vsc_lpr[0].handle_lpr[0].parkingID);
	//RAW2File("/aaa/bbb.obi",tttt, 111,222,8,IMG_FMT_BITMAP,0);
	tttt = (char *)malloc(32*sizeof(char));
	free(tttt);
	free(tttt);*/
	system("echo main over > ./boot.log");
	while(1) 
	{
		if(mcu_wdt_reset(WDT_MARGIN_TIME) < 0)
		{Write_Log(0,"[MAIN] mcu_wdt_reset fail\n");}
		sleep(5);
	}

	return 0;

	/*
	在libOBSysInf.a我有porting以下原Window相容的function. 您可直接使用

	int GetPrivateProfileInt( char *section, char *entry, int def, char *file_name );  //not work
	int GetPrivateProfileString( char *section, char *entry, char *def, char *buffer, int buffer_len, char *file_name );
	int WritePrivateProfileString( char *section, char *entry, char *buffer, char *file_name );
	*/
}

