#include "SocketServer.h"
#include <fcntl.h>



//#define dbg		1

#define BSIZE    0x500000
#define MSG_SIZE 0x100000
#define SOCK_NO  5
#define PARK_MAX 64

FILE*			dbg_file;
static char		dbg_sting[512];

typedef struct
{
	result_info ans[MAX_CH];
	int			send_flag;
}match_ans;

typedef struct
{
	int				park_id;
	
	struct tm		t_start;
	struct tm		t_end;

}vsc_match_info;

typedef struct
{
	char	plate[32];

	u32		timestamp;
	u32		micro_sec;

}evt_lnfo;

typedef struct
{
	u32		start_timestamp;
	u32		start_micro_sec;

	int		list_inuse;
	int		vs_index;
	int		tag;

	pthread_mutex_t ans_list_mutex;

	int     state_changed;
	evt_lnfo answer[32];
}ch_ans;

typedef struct
{
	int			park_id;

	int			base_cout;

	int			top_cout;

	char		temp_plate[32];

	char		Max_plate[32];
	
	result_info *vote_list;

	result_info *empty_ans;

}sock_client_t;

typedef struct {

	int		sd;

	u32     magic;
	u32     ip;
	int     port;
	
	int		connnected;
	int		first_conn;

	int		send_fg[MAX_CH];
	int		PS_flag[MAX_CH];
	int		send_max[MAX_CH];
	
	pthread_mutex_t *list_mutex[MAX_CH];

	sock_client_t	sck[MAX_CH];
} gateway_c;

static int      park_list[32];

static BYTE		jbuf[5000000];

//static BYTE		tbuf[5000000];

static ch_ans	ans_list[PARK_MAX];

int				*vsf;

static int		Vs_flag;

static int		Total_Num;

static char		loc_ip[32];

static as_frame_t	aft;

static 	match_ans	temp_ans[MAX_CH];

//static char		log_msg[256];
static pthread_mutex_t client_mux;
extern unsigned char *g;

//Thread
static inline int create_new_thread(void* thrd, void *arg)
{
	pthread_t thread_id;
	char log_msg[256];
	if(pthread_create(&thread_id, NULL, thrd, arg) != 0) 
	{
		sprintf(log_msg,"[IPC] [THREAD] %s\n",strerror(errno));
		Write_Log(0,log_msg);
		return -1;
	}

	pthread_detach(thread_id);

	return 0;
}

// -------------------------------------------------------------------------------
/*static int recv_server_return(int sd , CMD_H *fh, char* buf)
{
	fd_set rds;
	struct timeval tv;
	int retval;

	FD_ZERO(&rds);
	FD_SET(sd, &rds);

	tv.tv_sec = 0;
	tv.tv_usec = 100000;
}*/

int   list_sort(int ch_id, int *sort_list, result_info *ans,int base_cout)
{
	int tmp_cout[MAX_CH]; 
	int cout = 0;
	int x;
	char		log_msg[MSG_SIZE];

	memset(tmp_cout, 0, MAX_CH);

	for (x=0 ; x<MAX_CH ; x++)
	{
		sprintf(log_msg,"[GWC] [SORT] [ch%04d] <%ld>, ans-list[%d]. \n", ch_id, ans, x);
		Write_Log(1,log_msg);
		if (ans[x].cout == 0 )
			break;

		tmp_cout[x] = ans[x].cout;	
		cout++;
	}

	sprintf(log_msg,"[GWC] [SORT] [ch%04d] <%ld>, list cout=%d. \n", ch_id, ans, cout);
	Write_Log(1,log_msg);

	int y;
	int top;
	for (x=0 ; x<cout ; x++)
	{		
		top = 0;
		for (y=0 ; y<cout ; y++)
		{
			if(top < tmp_cout[y])
			{
				top = tmp_cout[y];
				sort_list[x] = y;
			}
		}

		sprintf(log_msg,"[GWC] [SORT] [ch%04d] <%ld> tmp_list[%d] = %s(%d) . \n", ch_id, ans, x, ans[sort_list[x]].plate, ans[sort_list[x]].cout);
		Write_Log(1,log_msg);
		tmp_cout[sort_list[x]] = 0;
	}


	return cout;
}

static int Send_cmd(void *hgc, char *sck_cmd )
{
	gateway_c	*gc = (gateway_c *) hgc;

	CMD_H		*fh;
	CMD_H		s_cm;

	fd_set	  rds;
	struct timeval tv;

	int			ret;
	char		msg[MSG_SIZE];
	char		log_msg[MSG_SIZE];

	//printf("[IPC] [GWC] [CMD] Send msg - %s. \n", sck_cmd);
	
	memset( &(s_cm), 0, CMD_H_SZ );
	s_cm.dwMagic = CMD_MAGIC;
	s_cm.dwVersion = CMD_VERSION;
	s_cm.dwLen = strlen(sck_cmd);

	memset( msg, 0, MSG_SIZE );
	memcpy(msg, &(s_cm), sizeof(CMD_H));
	sprintf(msg+sizeof(CMD_H), "%s", sck_cmd);
	sprintf(log_msg,"[IPC] [SCMD] [GWC] send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", msg+sizeof(CMD_H), s_cm.dwLen+sizeof(CMD_H), s_cm.dwMagic,  s_cm.dwVersion, s_cm.dwLen);
	Write_Log(0,log_msg);

	FD_ZERO(&rds);
	FD_SET(gc->sd, &rds);

	tv.tv_sec = 3;
	tv.tv_usec = 100000;
	while(1)
	{
		//Write_Log(0,"[IPC] [SCMD] [GWC] PINGTEST WHILE 1\n");
		int x = 0;
		ret = select(gc->sd+1, 0, &rds, 0, &tv);
		if (ret > 0)
		{
			break;
		}
		else if (ret <= 0 )
		{
			x++;
		}
	    if (x==3)
		{
			return -1;			
		}
	}

	if( (ret = send(gc->sd, msg, s_cm.dwLen+sizeof(CMD_H), 0)) < s_cm.dwLen+sizeof(CMD_H) )
	{
		sprintf(log_msg,"[IPC] [SCMD] [GWCE] send event list send_msg-%s(%d) fail ret=%d\n", msg+sizeof(CMD_H), s_cm.dwLen+sizeof(CMD_H), ret);
		Write_Log(0,log_msg);
		return -1;
	}

	int x = 0;
	do
	{
		//Write_Log(0,"[IPC] [SCMD] [GWC] PINGTEST DO WHILE 2\n");
		FD_ZERO(&rds);
		FD_SET(gc->sd, &rds);

		tv.tv_sec = 1;
		tv.tv_usec = 100000;

		ret = select(gc->sd+1, &rds, 0, 0, &tv);

		if(ret < 0) 
		{
			if(errno != EINTR)
			{
				gc->connnected = 0;
				sprintf(log_msg,"[IPC] [SCMD] [GWCE] %s(%08x) select error: %s\n", gc->ip, inet_addr(gc->ip), strerror(errno));
				Write_Log(0,log_msg);
				return -2;
			}
			return -2;
		}
		else if(ret == 0 || !FD_ISSET(gc->sd, &rds))
		{
			sprintf(log_msg,"[IPC] [SCMD] [GWC] %s(%08x) select return = %d (%d): %s\n", gc->ip, inet_addr(gc->ip),ret, x, strerror(errno));
			Write_Log(1,log_msg);
			x++;
		}

		if(x >= 30)
			return -2;
	}while(ret == 0);

	fh = (CMD_H*)msg;
	ret = recv(gc->sd, msg, MSG_SIZE, 0);

	if (ret < 0)
	{
		sprintf(log_msg,"[IPC] [SCMD] [GWCE] [RECV] recv msg from server faill ret=%d. \n", ret);
		Write_Log(0,log_msg);
		return -3;
	}
	else if (ret == 0)
	{
		sprintf(log_msg,"[IPC] [SCMD] [GWCE] [RECV] ret = 0 .\n");
		Write_Log(0,log_msg);
	}
	else
	{
		sprintf(log_msg,"[IPC] [SCMD] [GWC] [RECV] recv msg-%s(%d) from server.\n", msg+sizeof(CMD_H), fh->dwLen);
		Write_Log(0,log_msg);
	}
	return 1;
}

// Socket client for gateway
static void *hgc_thread(void *arg)
{
	gateway_c *gc = (gateway_c *)arg;
	gc->sd = -1;

	CMD_H		 *fh;
	CMD_H		 s_cm;
	int			fh_len;

	//socket select
	fd_set		rds;
	struct		timeval tv;
	int			retval, ret;

	int			x,i,q;
	int			y,j;
	int			re_count = 0;

	char		cmd_header[MSG_SIZE];
	char		ping_header[MSG_SIZE];
	char		plate_string[256];
	char		vs_string[MSG_SIZE];
	char		msg[MSG_SIZE];

	int			sor_list[MAX_CH];
	char		log_msg[MSG_SIZE];
	result_info ans[MAX_CH];

	sprintf(log_msg,"[IPC] [HGC] [GWC] thread start(%08X).... \n", inet_addr(gc->ip));
	Write_Log(1,log_msg);

	for(x=0; x<MAX_CH ; x++)
	{
		if( gc->sck[x].park_id != -1)
			gc->send_fg[x] = 1;
	}
_hgc_init_:
	sprintf(log_msg,"[IPC] [HGC] [GWC] init conn ser-ip(%08X). \n", inet_addr(gc->ip));
	Write_Log(1,log_msg);
	for(x=0; x<MAX_CH ; x++)
	{
		gc->PS_flag[x] = 0;
		gc->send_max[x] = 0;
	}
	Vs_flag = 0;
	if(gc->sd >= 0) {
		close(gc->sd);
		gc->sd = -1;
	}

	if(!gc->ip) {
		sprintf(log_msg,"[IPC] [HGC] [GWCE] invalid ip (%08X). \n", inet_addr(gc->ip));
		Write_Log(0,log_msg);
		goto _hgc_exit_;
	}

	gc->sd = socket(PF_INET, SOCK_STREAM, 0);
	if(gc->sd < 0) {
		sprintf(log_msg,"[IPC] [HGC] [GWCE] (%08X) socket error: %s", inet_addr(gc->ip), strerror(errno));
		Write_Log(0,log_msg);
		goto _hgc_exit_;
	}

	sprintf(log_msg,"[IPC] [HGC] [GWC] %s connecting.\n", gc->ip);
	Write_Log(0,log_msg);
	struct sockaddr_in  peer;
	peer.sin_family = PF_INET;
	peer.sin_port = htons(gc->port);
	peer.sin_addr.s_addr = inet_addr(gc->ip);
	while(connect(gc->sd, (struct sockaddr *)&peer, sizeof(peer)) < 0) 
	{
		sprintf(log_msg,"[IPC] [HGC] [GWCE] %s(%08X) connect error: %s\n", gc->ip, inet_addr(gc->ip), strerror(errno));
		Write_Log(0,log_msg);
		if(!gc->ip)//|| !gc->magic)
			goto _hgc_exit_;
		usleep(1000);
	}

	sprintf(log_msg,"[IPC] [HGC] [GWC] %s connected.\n", gc->ip);
	Write_Log(0,log_msg);
	gc->connnected = 1;
	gc->magic = CMD_MAGIC;

	while(gc->magic == CMD_MAGIC) 
	{
		//List check
		for(x=0 ; x<MAX_CH ; x++)
		{
			if (gc->send_max[x]>=1)
			{
				ret = list_sort(gc->sck[x].park_id, sor_list, temp_ans[x].ans,gc->sck[x].base_cout);
				for(y=0 ; y<MAX_CH ; y++)
				{
					sprintf(log_msg,"[IPC] [HGC] [GWC] [ch%04d] <%ld>, list%d - %s(%d) .\n", gc->sck[x].park_id, temp_ans[x].ans, y,temp_ans[x].ans[y].plate,temp_ans[x].ans[y].cout);
					Write_Log(1,log_msg);
				}
				if (gc->send_max[x]==2 && strcmp(gc->sck[x].Max_plate,temp_ans[x].ans[0].plate) != 0)
				{
					strcpy(gc->sck[x].Max_plate,temp_ans[x].ans[0].plate);
					if ( (retval=Send_cmd(gc, "|Ping|")) < 1)
					{
						Write_Log(0,"[IPC] [HGC] [GWCE] send ping fail\n");
						goto _hgc_init_;
					}
					sprintf(cmd_header,"|SendMaxAns|%d|1|%s||",gc->sck[x].park_id,temp_ans[x].ans[0].plate);
					if ( (retval=Send_cmd(gc,cmd_header)) < 1)
					{
						Write_Log(0,"[IPC] [HGC] [GWCE] send SendMaxAns fail\n");
						goto _hgc_init_;
					}
					else
					{
						gc->send_max[x] = 0;
					}
				}									
				if(temp_ans[x].send_flag == 1)
				{//before clean Answer SendMaxAns
					if ( (retval=Send_cmd(gc, "|Ping|")) < 1)
					{
						Write_Log(0,"[IPC] [HGC] [GWCE] send ping fail\n");
						goto _hgc_init_;
					}
					temp_ans[x].send_flag = 0;
					for (q = 0;q<ret ; q++)
					{
						strcat(plate_string,temp_ans[x].ans[q].plate);
						strcat(plate_string,"|");
					}
					sprintf(cmd_header,"|SendMaxAns|%d|%d|%s|",gc->sck[x].park_id,ret,plate_string);
					memset(plate_string,0,strlen(plate_string));
					if ( (retval=Send_cmd(gc,cmd_header)) > 0)
					{
						sprintf(log_msg,"[IPC] [HGC] [GWC] [SENL] [ch%04d] send event sucess. \n", gc->sck[x].park_id);
						Write_Log(0,log_msg);
						gc->send_max[x] = 0;
						memset(gc->sck[x].Max_plate,0,strlen(gc->sck[x].Max_plate));
					}	
					else
					{
						sprintf(log_msg,"[IPC] [HGC] [GWCE] [SENL] [ch%04d] send event fail. \n", gc->sck[x].park_id);
						Write_Log(0,log_msg);
						goto _hgc_init_;
					}
				}
			}

			if( (gc->send_fg[x]==1) && (gc->sck[x].park_id!=-1) )// && (gc->sck[x].park_id!=-1) )
			{

				memset(ans, 0 ,sizeof(result_info)*MAX_CH);
				
				//pthread_mutex_lock( gc->list_mutex[x] );
				memcpy(ans, gc->sck[x].vote_list, sizeof(result_info)*MAX_CH);
				for(y=0 ; y<MAX_CH ; y++)
				{
					//if(ans[y].cout != 0)
						sprintf(log_msg,"[IPC] [HGC] [GWC] [ch%04d] <%ld>, list%d - %s(%d) .\n", gc->sck[x].park_id, gc->sck[x].vote_list, y, gc->sck[x].vote_list[y].plate, gc->sck[x].vote_list[y].cout);
						Write_Log(1,log_msg);
				}
				//pthread_mutex_unlock( gc->list_mutex[x] );

				sprintf(log_msg,"[IPC] [HGC] [GWC] [ch%04d] <%ld>, call sort <%ld> .\n", gc->sck[x].park_id, gc->sck[x].vote_list, ans);
				Write_Log(1,log_msg);
				ret = list_sort(gc->sck[x].park_id, sor_list, ans,gc->sck[x].base_cout);
				
				if(ans[sor_list[0]].cout == 0)
				{//empty event
					if ( (retval=Send_cmd(gc, "|Ping|")) < 1)
					{
						Write_Log(0,"[IPC] [HGC] [GWCE] send ping fail\n");
						goto _hgc_init_;
					}
					if (gc->PS_flag[x] == 1)
					{
						sprintf(cmd_header,"|SendParkingStatus|%d|0||",gc->sck[x].park_id);
						if ( (retval=Send_cmd(gc,cmd_header)) < 1)
						{
							Write_Log(0,"[IPC] [HGC] [GWCE] send SendParkingStatus fail\n");
							goto _hgc_init_;
						}
						gc->PS_flag[x] = 0;
					}
					retval = Send_List(gc, gc->sck[x].park_id, 0, 0, 0, 0, gc->sck[x].empty_ans, loc_ip, 1);
					if( retval < 0)
					{
						sprintf(log_msg,"[IPC] [HGC] [GWCE] [SENL] [ch%04d] send empty event fail. \n", gc->sck[x].park_id);
						Write_Log(0,log_msg);
						goto _hgc_init_;
					}
					else 
					{
						memset(temp_ans[x].ans, 0 ,sizeof(result_info)*MAX_CH);
						sprintf(log_msg,"[IPC] [HGC] [GWC] [SENL] retval > 0 \n", gc->sck[x].park_id);
						Write_Log(0,log_msg);
					}
					gc->send_fg[x] = 0;
				}
				else {
						if (gc->PS_flag[x] == 0)
						{
							sprintf(cmd_header,"|SendParkingStatus|%d|1||",gc->sck[x].park_id);
							if ( (retval=Send_cmd(gc,cmd_header)) < 1)
							{
								Write_Log(0,"[IPC] [HGC] [GWCE] send SendParkingStatus fail\n");
								goto _hgc_init_;
							}
							gc->PS_flag[x] = 1;
						}
						retval = Send_List(gc, gc->sck[x].park_id, gc->sck[x].base_cout, &sor_list, ret, &ans, gc->sck[x].empty_ans, loc_ip, 0);
						if (retval > 0)
						{
							sprintf(log_msg,"[IPC] [HGC] [GWC] [SENL] [ch%04d] send event sucess. \n", gc->sck[x].park_id);
							Write_Log(0,log_msg);
							gc->send_fg[x] = 0;
						}	
						else
						{
							sprintf(log_msg,"[IPC] [HGC] [GWCE] [SENL] [ch%04d] send event fail. \n", gc->sck[x].park_id);
							Write_Log(0,log_msg);
							goto _hgc_init_;
						}
					}
			}
		}

		if (Vs_flag == 1)
		{	
			int		vs;
			char	vs_temp[32];
			sprintf(vs_string,"|ReturnVsState|");
			for (vs = 0 ; vs<=Total_Num ; vs++)
			{
				sprintf(vs_temp,"%d|",vsf[vs]);
				strcat(vs_string,vs_temp);
			}
			
			sprintf(log_msg,"%s\n",vs_string);
			Write_Log(1,log_msg);
			if ( (retval=Send_cmd(gc, vs_string)) < 1)
			{
				Write_Log(0,"[IPC] [HGC] [GWCE] send Vs_State fail\n");
				goto _hgc_init_;
			}
			Vs_flag = 0;
		}

		x = 0;

		FD_ZERO(&rds);
		FD_SET(gc->sd, &rds);
		tv.tv_sec = 30;
		tv.tv_usec = 100000;

		retval = select(gc->sd+1, &rds, 0, 0, &tv);

		if(retval < 0) 
		{
			if(errno != EINTR) {
				gc->connnected = 0;
				sprintf(log_msg,"[IPC] [HGC] [GWCE] %s(%08x) select error: %s\n", gc->ip, inet_addr(gc->ip), strerror(errno));
				Write_Log(0,log_msg);
				goto _hgc_init_;
			}
			goto _hgc_init_;
		}
		else if(retval == 0 || !FD_ISSET(gc->sd, &rds))
		{
			//sprintf(log_msg,"[IPC] [HGC] [GWC] %s(%08x) select return=0 (%d): Not receive any CMD form Gateway\n", gc->ip, inet_addr(gc->ip), x);
			sprintf(log_msg,"[IPC] [HGC] [GWC] Not receive any CMD form Gateway\n");
			Write_Log(1,log_msg);
			//goto _hgc_init_;
			sleep(1);
			continue;
		}
		else if ( retval > 0)
		{
			memset(msg,0,MSG_SIZE);
			fh = (CMD_H*)msg;
			ret = recv(gc->sd, msg, sizeof(CMD_H), 0);

			if (ret < 0)
			{
				sprintf(log_msg,"[IPC] [HGC] [GWCE] [RECV] recv msg from server fail ret=%d. \n", ret);
				Write_Log(0,log_msg);
				goto _hgc_init_;
			}
			else if (ret == 0)
			{
				sprintf(log_msg,"[IPC] [HGC] [GWCE] ret = 0 .\n");
				Write_Log(0,log_msg);
			}
			else
			{
				ret = recv(gc->sd, msg, fh->dwLen, 0);
				strncpy(ping_header, msg ,6);
				sprintf(log_msg,"[IPC] [HGC] [GWC] [RECVP] recv msg-%s(%d) from server.\n", ping_header, fh->dwLen);
				Write_Log(0,log_msg);
				if( strcmp(ping_header, "|Ping|") == 0)
				{
					sprintf(ping_header, "|Ping_OK|");
					memset( &(s_cm), 0, CMD_H_SZ );
					s_cm.dwMagic = CMD_MAGIC;
					s_cm.dwVersion = CMD_VERSION;
					s_cm.dwLen = strlen(ping_header);

					memset(msg, 0, MSG_SIZE );
					memcpy(msg, &(s_cm), sizeof(CMD_H));
					sprintf(msg+sizeof(CMD_H), "%s", ping_header);
					sprintf(log_msg,"[IPC] [HGC] [GWC] [CMDP] send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", msg+sizeof(CMD_H), s_cm.dwLen+sizeof(CMD_H), s_cm.dwMagic,  s_cm.dwVersion, s_cm.dwLen);
					Write_Log(1,log_msg);

					if( (ret = send(gc->sd, msg, s_cm.dwLen+sizeof(CMD_H), 0)) < s_cm.dwLen+sizeof(CMD_H) )
					{
						sprintf(log_msg,"[IPC] [HGC] [GWCE] [SENDP] send_msg-%s(%d) fail ret=%d\n", msg+sizeof(CMD_H), s_cm.dwLen+sizeof(CMD_H), ret);
						Write_Log(0,log_msg);
						goto _hgc_init_;
					}
				}
				memset(ping_header,0,MSG_SIZE);
				//if (strcmp(cmd_header,"|Reboot|")==0)
				//{
				//	sprintf(cmd_header, "|Reboot_OK|");
				//	memset( &(s_cm), 0, CMD_H_SZ );
				//	s_cm.dwMagic = CMD_MAGIC;
				//	s_cm.dwVersion = CMD_VERSION;
				//	s_cm.dwLen = strlen(cmd_header);

				//	memset( msg, 0, MSG_SIZE );
				//	memcpy(msg, &(s_cm), sizeof(CMD_H));
				//	sprintf(msg+sizeof(CMD_H), "%s", cmd_header);
				//	sprintf(log_msg,"[IPC] [HGC] [GWC] [CMDR] send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", msg+sizeof(CMD_H), s_cm.dwLen+sizeof(CMD_H), s_cm.dwMagic,  s_cm.dwVersion, s_cm.dwLen);
				//	Write_Log(1,log_msg);

				//	if( (ret = send(gc->sd, msg, s_cm.dwLen+sizeof(CMD_H), 0)) < s_cm.dwLen+sizeof(CMD_H) )
				//	{
				//		sprintf(log_msg,"[IPC] [HGC] [GWCE] [SENDR] send_msg-%s(%d) fail ret=%d\n", msg+sizeof(CMD_H), s_cm.dwLen+sizeof(CMD_H), ret);
				//		Write_Log(0,log_msg);
				//		goto _hgc_init_;
				//	}
				//	sleep(5);
				//	system("reboot");
				//}
			}
		}
		
		sleep(5);
	}

_hgc_exit_:
	gc->connnected = 0;

	sprintf(log_msg,"[IPC] [HGC] [GWC] free gc->sd\n");
	Write_Log(0,log_msg);
	if(gc->sd >= 0)
		close(gc->sd);

	sprintf(log_msg,"[IPC] [HGC] [GWC] <%08x>thread stopped\n", gc->ip);
	Write_Log(0,log_msg);
	while(gc->magic == CMD_MAGIC)
		usleep(100000);

	//printf("[IPC] [GWC] free gc");
	Write_Log(0,"[IPC] [HGC] [GWC] free gc\n");
	free(gc);

	sprintf(log_msg,"[IPC] [HGC] [GWC] <%08x>thread closed\n", gc->ip);
	Write_Log(0,log_msg);
	return 0;
}

// -------------------------------------------------------------------------------

time_t	Convert_Time_string(char *t_string)
{
	struct  tm	t_tmp;

	char		tmp_string[32];
	char		log_msg[MSG_SIZE];
	memset(tmp_string, 0 ,32);
	strncpy(tmp_string, t_string, 4);
	t_tmp.tm_year = ( atoi(tmp_string) - 1900);

	memset(tmp_string, 0 ,32);
	strncpy(tmp_string, t_string+4, 2);
	t_tmp.tm_mon = ( atoi(tmp_string) - 1 );

	memset(tmp_string, 0 ,32);
	strncpy(tmp_string, t_string+6, 2);
	t_tmp.tm_mday = atoi(tmp_string);

	memset(tmp_string, 0 ,32);
	strncpy(tmp_string, t_string+8, 2);
	t_tmp.tm_hour = atoi(tmp_string);

	memset(tmp_string, 0 ,32);
	strncpy(tmp_string, t_string+10, 2);
	t_tmp.tm_min = atoi(tmp_string);

	memset(tmp_string, 0 ,32);
	strncpy(tmp_string, t_string+12, 2);
	t_tmp.tm_sec = atoi(tmp_string);

	return mktime(&t_tmp);
}

int		Command_Analysis(void *hsock_param)
{
	//////////////////////////////////////////////////////////////////////////
	//cmd id --> hello=1, AskFileList=2, AskEventList=3, AskEventData=4, AskVideoData=5, AskSnapshot=6
	//////////////////////////////////////////////////////////////////////////
	socket_client	*pSockClient = (socket_client*) hsock_param;

	int			x, len;
	char		*pstr[10];
	char		*cmd_header, *savePoint, *tmp_cmd;
	char		log_msg[MSG_SIZE];
	

	char		tmp_string[8];
	char		*tmpMessage = pSockClient->msg;

	
	//strcpy(tmpMessage, pSockClient->msg);
	len = strlen(tmpMessage);

	cmd_header = tmpMessage+1;
	savePoint = tmpMessage+1;

	x = 0;
	while( (tmp_cmd=strstr(savePoint, "|")) != 0)
	{
		*tmp_cmd = 0;

		pstr[x] = savePoint;
		savePoint = tmp_cmd+1;
		++x;
	}

	//printf("[Analysis] cmd=%s, x=%d \n", cmd_header, x);
	if (strcmp(cmd_header,"Hello") == 0)				//|Hello|APName|
	{
		if (x == 2)
		{
			strcpy(pSockClient->cmd_info.cName, pstr[1]);
			
			pSockClient->cmd_info.cmd_id = 1;

			sprintf(log_msg,"[IPC] [Analysis] cmd=%s, name=%s \n", cmd_header, pSockClient->cmd_info.cName);
			Write_Log(0,log_msg);
			return 1;
		}
	}
	else if (strcmp(cmd_header,"AskFileList") == 0)	//|AskFileList|chNo|TimeFrom|TimeTo|Option|
	{
		if(x == 5)
		{
			pSockClient->cmd_info.ch_id = atoi(pstr[1]);
			strcpy(pSockClient->cmd_info.cStrat_time, pstr[2]);
			strcpy(pSockClient->cmd_info.cEnd_time, pstr[3]);
			strcpy(pSockClient->cmd_info.cOption, pstr[4]);

			pSockClient->cmd_info.cmd_id = 2;

			sprintf(log_msg,"[IPC] [Analysis] cmd=%s, id=%d, s-time=%s, e-time=%s, opt=%s \n", cmd_header, pSockClient->cmd_info.ch_id, pSockClient->cmd_info.cStrat_time,pSockClient->cmd_info.cEnd_time, pSockClient->cmd_info.cOption);
			Write_Log(0,log_msg);	
			return 2;
		}
	}
	else if (strcmp(cmd_header,"AskEventList") == 0)	//|AskEventList|File|TimeFrom|TimeTo|Type|Option|
	{
		if(x == 6)
		{
			strcpy(pSockClient->cmd_info.cFile, pstr[1]);
// 			strcpy(pSockClient->cmd_info.cStrat_time, pstr[2]);
// 			strcpy(pSockClient->cmd_info.cEnd_time, pstr[3]);
 			strcpy(pSockClient->cmd_info.cData_type, pstr[4]);
			strcpy(pSockClient->cmd_info.cOption, pstr[5]);

			memcpy(tmp_string, pSockClient->cmd_info.cFile+2, 4);
			pSockClient->cmd_info.ch_id = atoi(tmp_string);

			savePoint = pSockClient->cmd_info.cFile;
			x = 0;
			while( (tmp_cmd=strstr(savePoint, "_")) != 0)
			{
				*tmp_cmd = 0;

				pstr[x] = savePoint;
				savePoint = tmp_cmd+1;
				++x;
			}
			strcpy(pSockClient->cmd_info.cStrat_time, pstr[1]);
			strcpy(pSockClient->cmd_info.cEnd_time, pstr[2]);

			pSockClient->cmd_info.cmd_id = 3;

			sprintf(log_msg,"[IPC] [Analysis] cmd=%s, file=%s, s-time=%s, e-time=%s, opt=%s ,chid=%d. \n", cmd_header, pSockClient->cmd_info.cFile, pSockClient->cmd_info.cStrat_time,	pSockClient->cmd_info.cEnd_time, pSockClient->cmd_info.cOption, pSockClient->cmd_info.ch_id);
			Write_Log(0,log_msg);

#ifdef dbg
			sprintf(dbg_sting, "[Analysis] AskEventList -- cmd=%s, file=%s, s-time=%s, e-time=%s, opt=%s ,chid=%d. \n", cmd_header, pSockClient->cmd_info.cFile, pSockClient->cmd_info.cStrat_time,	pSockClient->cmd_info.cEnd_time, pSockClient->cmd_info.cOption, pSockClient->cmd_info.ch_id);
			fwrite(dbg_sting, sizeof(char), strlen(dbg_sting), dbg_file);
#endif
			return 3;
		}
	}
	else if (strcmp(cmd_header,"AskEventData") == 0)	//|AskEventData|File|Event|Type|Option|
	{
		if(x == 5)
		{
			strcpy(pSockClient->cmd_info.cFile, pstr[1]);
			strcpy(pSockClient->cmd_info.cEvent, pstr[2]);
			strcpy(pSockClient->cmd_info.cData_type, pstr[3]);
			strcpy(pSockClient->cmd_info.cOption, pstr[4]);

			pSockClient->cmd_info.cmd_id = 4;

			sprintf(log_msg,"[IPC] [Analysis] cmd=%s, file=%s, evt=%s, type=%s, opt=%s \n", cmd_header, pSockClient->cmd_info.cFile, pSockClient->cmd_info.cEvent,
			pSockClient->cmd_info.cData_type, pSockClient->cmd_info.cOption);
			Write_Log(0,log_msg);
			return 4;
		}
	}
	else if (strcmp(cmd_header,"AskVideoData") == 0)	//|AskVideoData|File|TimeFrom|TimeTo|Option|
	{
		if(x == 5)
		{
			strcpy(pSockClient->cmd_info.cFile, pstr[1]);
			strcpy(pSockClient->cmd_info.cStrat_time, pstr[2]);
			strcpy(pSockClient->cmd_info.cEnd_time, pstr[3]);
			strcpy(pSockClient->cmd_info.cOption, pstr[4]);

			pSockClient->cmd_info.cmd_id = 5;

			sprintf(log_msg,"[IPC] [Analysis] cmd=%s, file=%s, s-time=%s, e-time=%s, opt=%s \n", cmd_header, pSockClient->cmd_info.cFile, pSockClient->cmd_info.cStrat_time,
			pSockClient->cmd_info.cEnd_time, pSockClient->cmd_info.cOption);
			Write_Log(0,log_msg);
			return 5;
		}
	}
	else if (strcmp(cmd_header,"AskSnapshot") == 0)		//|AskSnapshot|ChNo|Option|
	{
		if(x == 3)
		{
			pSockClient->cmd_info.ch_id = atoi(pstr[1]);
			strcpy(pSockClient->cmd_info.cOption, pstr[2]);

			pSockClient->cmd_info.cmd_id = 6;

			sprintf(log_msg,"[IPC] [Analysis] cmd=%s, id=%d, opt=%s \n", cmd_header, pSockClient->cmd_info.ch_id, pSockClient->cmd_info.cOption);
			Write_Log(0,log_msg);
			return 6;
		}
	}

	return 0;
}

int		Prepare_Msg(void *hcmd_param)
{
	//get time sample : http://fp-moon.iteye.com/blog/964377

	socket_client	*pSockClient = (socket_client*) hcmd_param;
	//rec_config_t	*reconfig = pSockClient->reconfig;

	char			*t, *t1;
	char		log_msg[MSG_SIZE];

	void			*hReader;
	struct tm		t_start;
	struct tm		t_end;
	struct tm		*t_tmp_s;
	struct tm       *t_tmp_e;
	time_t			tmp_time_s;
	time_t			tmp_time_e;

	obr_result_t	result;
	int				ev_counter=0, ev_len=0, ev_dcheck_counter=0;

	char			tmp_string[64];
	char			tmp[64];
	int				ret;
	int				tmp_int;
	int				debug_flag;			

	//snap
	u32				now;
	struct timeval  tt;
	int				jsize;
	//void			*jbuf;

	GetPrivateProfileString( "System", "Debug_Mod", "0", tmp, 32, "3531.ini" );
	debug_flag = atoi(tmp);

	if (strcmp(pSockClient->cmd_info.cStrat_time, "") == 0)
	{
		t_start.tm_year = 2000-1900;
		t_start.tm_mon = 0;
		t_start.tm_mday = 1;
		t_start.tm_hour = 0;
		t_start.tm_min = 0;
		t_start.tm_sec = 0;

		pSockClient->cmd_info.time_start = mktime(&t_start);

		sprintf(log_msg,"[IPC] [Prepare_Msg] [ch%04d] [SEND] [WARNING] not find start time .\n", pSockClient->cmd_info.ch_id);
		Write_Log(0,log_msg);
	}
	else
	{
		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time, 4);
		t_start.tm_year = ( atoi(tmp_string) - 1900);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time+4, 2);
		t_start.tm_mon = ( atoi(tmp_string) - 1 );

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time+6, 2);
		t_start.tm_mday = atoi(tmp_string);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time+8, 2);
		t_start.tm_hour = atoi(tmp_string);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time+10, 2);
		t_start.tm_min = atoi(tmp_string);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time+12, 2);
		t_start.tm_sec = atoi(tmp_string);

		pSockClient->cmd_info.time_start = mktime(&t_start);
	}
	
	if (strcmp(pSockClient->cmd_info.cEnd_time, "") == 0)
	{		
		time(&(pSockClient->cmd_info.time_end));
		gmtime_r(&(pSockClient->cmd_info.time_end), &t_end);
		//millisecond
		//gettimeofday(&(pSockClient->cmd_info.millsec_end), NULL);
		//double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecond

		sprintf(log_msg,"[IPC] [Prepare_Msg] [ch%04d] [SEND] [WARNING] not find end time. \n ", pSockClient->cmd_info.ch_id);
		Write_Log(0,log_msg);
	}
	else
	{
		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time, 4);
		t_end.tm_year = ( atoi(tmp_string) - 1900);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time+4, 2);
		t_end.tm_mon = ( atoi(tmp_string) - 1 );

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time+6, 2);
		t_end.tm_mday = atoi(tmp_string);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time+8, 2);
		t_end.tm_hour = atoi(tmp_string);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time+10, 2);
		t_end.tm_min = atoi(tmp_string);

		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time+12, 2);
		t_end.tm_sec = atoi(tmp_string);

		pSockClient->cmd_info.time_end = mktime(&t_end);
	}

	sprintf(log_msg,"[IPC] [Prepare_Msg] [ch%04d] [SEND], s-time=%04d%02d%02d%02d%02d%02d = %d \n", pSockClient->cmd_info.ch_id, t_start.tm_year+1900, t_start.tm_mon+1, t_start.tm_mday, t_start.tm_hour, t_start.tm_min, t_start.tm_sec, pSockClient->cmd_info.time_start);
	Write_Log(0,log_msg);

	sprintf(log_msg,"[IPC] [Prepare_Msg] [ch%04d] [SEND], e-time=%04d%02d%02d%02d%02d%02d = %d \n", pSockClient->cmd_info.ch_id, t_end.tm_year+1900, t_end.tm_mon+1, t_end.tm_mday, t_end.tm_hour, t_end.tm_min, t_end.tm_sec, pSockClient->cmd_info.time_end);
	Write_Log(0,log_msg);

	S_CMD	s_cm;

	char	*send_msg = pSockClient->msg;

	memset( send_msg, 0 ,100000);
 	memset( &(s_cm.h), 0, CMD_H_SZ );
 	s_cm.h.dwMagic = CMD_MAGIC;
 	s_cm.h.dwVersion = CMD_VERSION;

	FILE *fp = 0;
	//switch case ask command
	switch(pSockClient->cmd_info.cmd_id)
	{
	case 1://|Hello_OK|APName|Status(0/-1)|
		sprintf(send_msg, "|Hello_OK|iNVR|0|");
		s_cm.h.dwLen = strlen(send_msg);

		sprintf(log_msg,"[IPC] [Prepare_Msg] [ch%04d] [SEND] send hello send_msg=%s, msg-len=%d\n", pSockClient->cmd_info.ch_id, send_msg, s_cm.h.dwLen);
		Write_Log(0,log_msg);

		if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ)
		{
			sprintf(log_msg,"[IPC] [Prepare_Msg] [ch%04d] [SENDE] send hello header(%d) fail, ret=%d\n", pSockClient->cmd_info.ch_id, CMD_H_SZ, ret);
			Write_Log(0,log_msg);
			return -11;
		}
		else if( (ret = send(pSockClient->conn_client, send_msg, s_cm.h.dwLen, 0)) < s_cm.h.dwLen)
		{
			sprintf(log_msg,"[IPC] [Prepare_Msg] [ch%04d] [SENDE] send hello send_msg(%d) fail, ret=%d\n", pSockClient->cmd_info.ch_id, s_cm.h.dwLen, ret);
			Write_Log(0,log_msg);
			return -12;
		}
		break;

	case 2://|SendFileList|File#|File1|File2|…|
		//gmtime_r(&(pSockClient->cmd_info.time_end), &t_end);
		tmp_time_e = pSockClient->cmd_info.time_end;
		ev_counter = 0;
		while( (tmp_time_e-pSockClient->cmd_info.time_start) > 0)
		{
			tmp_time_e = tmp_time_e - 3600;
			ev_counter = ev_counter + 1;
		}

		sprintf(log_msg,"[IPC] send file list ev-count=%d. \n", ev_counter);
		Write_Log(0,log_msg);
		if( (strcmp(pSockClient->cmd_info.cOption, "DESC")) == 0) 
		{
			tmp_time_e = pSockClient->cmd_info.time_end;
			sprintf(send_msg, "|SendFileList|%d|", ev_counter);
			while(ev_counter > 0)
			{
				tmp_time_e = tmp_time_e - 3600;
				t_tmp_e = localtime(&tmp_time_e);
				//printf("[DBG] t_tmp_e=%04d%02d%02d%02d%02d%02d = %d \n",  t_tmp_e->tm_year+1900, t_tmp_e->tm_mon+1, t_tmp_e->tm_mday, t_tmp_e->tm_hour, t_tmp_e->tm_min, t_tmp_e->tm_sec, tmp_time_e);

				sprintf(tmp_string, "[ch%04d]_%04d%02d%02d%02d%02d%02d000_", pSockClient->cmd_info.ch_id, t_tmp_e->tm_year+1900, t_tmp_e->tm_mon+1, t_tmp_e->tm_mday, t_tmp_e->tm_hour, t_tmp_e->tm_min, t_tmp_e->tm_sec);
				strcat(send_msg, tmp_string);

				tmp_time_e = tmp_time_e + 3600;
				t_tmp_e = localtime(&tmp_time_e);
				//printf("[DBG] t_tmp_e=%04d%02d%02d%02d%02d%02d = %d \n",  t_tmp_e->tm_year+1900, t_tmp_e->tm_mon+1, t_tmp_e->tm_mday, t_tmp_e->tm_hour, t_tmp_e->tm_min, t_tmp_e->tm_sec, tmp_time_e);
				sprintf(tmp_string, "%04d%02d%02d%02d%02d%02d000_.h264|", t_tmp_e->tm_year+1900, t_tmp_e->tm_mon+1, t_tmp_e->tm_mday, t_tmp_e->tm_hour, t_tmp_e->tm_min, t_tmp_e->tm_sec);
				strcat(send_msg, tmp_string);

				tmp_time_e = tmp_time_e - 3600;
				ev_counter = ev_counter - 1;
			}
		}

		memset( &(s_cm.h), 0, CMD_H_SZ );
		s_cm.h.dwMagic = CMD_MAGIC;
		s_cm.h.dwVersion = CMD_VERSION;
		s_cm.h.dwLen = strlen(send_msg);
		sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", pSockClient->cmd_info.ch_id, send_msg, strlen(send_msg), s_cm.h.dwMagic,  s_cm.h.dwVersion, s_cm.h.dwLen);
		Write_Log(0,log_msg);

		if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ){
			sprintf(log_msg,"[IPC][SEND][ERROR] send file list header(%d) fail, ret=%d\n", CMD_H_SZ, ret);
			Write_Log(0,log_msg);
			}
		else if( (ret = send(pSockClient->conn_client, send_msg, s_cm.h.dwLen, 0)) < s_cm.h.dwLen){
			sprintf(log_msg,"[IPC][SEND][ERROR] send file list send_msg(%d) fail, ret=%d\n", s_cm.h.dwLen, ret);
			Write_Log(0,log_msg);
			}
		
/*		sprintf(send_msg, "|SendFileList|1|[ch%04d]_%04d%02d%02d%02d%02d%02d000_%04d%02d%02d%02d%02d%02d000_.h264|", pSockClient->cmd_info.ch_id, t_start.tm_year+1900, t_start.tm_mon+1, t_start.tm_mday, t_start.tm_hour, t_start.tm_min, t_start.tm_sec, t_end.tm_year+1900, t_end.tm_mon+1, t_end.tm_mday, t_end.tm_hour, t_end.tm_min, t_end.tm_sec);

		s_cm.h.dwLen =  strlen(send_msg);

		printf("[IPC][Send] msg=%s, msg-len=%d\n", send_msg, s_cm.h.dwLen);

		if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ)
			printf("[IPC][SEND][ERROR] send file list header(%d) fail, ret=%d\n", CMD_H_SZ, ret);
		else if( (ret = send(pSockClient->conn_client, send_msg, s_cm.h.dwLen, 0)) < s_cm.h.dwLen)
			printf("[IPC][SEND][ERROR] send file list send_msg(%d) fail, ret=%d\n", s_cm.h.dwLen, ret);
*/		break;

	case 3://|SendEventList|Event#|Event1|Event2|…|
		pthread_mutex_lock(&ans_list[pSockClient->cmd_info.ch_id-1].ans_list_mutex);
		while(ans_list[pSockClient->cmd_info.ch_id-1].list_inuse == 1)
		{
			//pthread_mutex_unlock(&ans_list[parking_id-1].ans_list_mutex);
			//return -1;
			usleep(50000);
		}
		ans_list[pSockClient->cmd_info.ch_id-1].list_inuse = 1;
		pthread_mutex_unlock(&ans_list[pSockClient->cmd_info.ch_id-1].ans_list_mutex);

		sprintf(log_msg,"[IPC][SEND][ALLOC] [ch%04d], e-time=%04d%02d%02d%02d%02d%02d = %d \n", pSockClient->cmd_info.ch_id, t_end.tm_year+1900, t_end.tm_mon+1, t_end.tm_mday, t_end.tm_hour, t_end.tm_min, t_end.tm_sec, pSockClient->cmd_info.time_end);
		Write_Log(0,log_msg);
		ev_counter = ev_len = 0;
		for(tmp_int=0 ; tmp_int<MAX_CH ; tmp_int++)
		{
			sprintf(log_msg,"[IPC] ev-[ch%04d] search ans_list[%d] time=%d. \n",pSockClient->cmd_info.ch_id, tmp_int, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].timestamp);
			Write_Log(0,log_msg);
			if(ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].timestamp != 0)
			{
				ev_counter = ev_counter + 1;
				ev_len = ev_len + strlen(ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].plate);
				sprintf(log_msg,"[IPC] ev-[ch%04d], ans_list[%d] plate=%s. \n", pSockClient->cmd_info.ch_id, tmp_int, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].plate);
				Write_Log(0,log_msg);
			}
			else
				break;
		}

/*		if( (hReader = obr_alloc(park_list[pSockClient->cmd_info.ch_id-1], pSockClient->cmd_info.ch_id, pSockClient->cmd_info.time_start, pSockClient->cmd_info.time_end)) != 0)
		{
			if( (strcmp(pSockClient->cmd_info.cOption, "DESC")) == 0) 
				pSockClient->cmd_info.option = 1;
			else
				pSockClient->cmd_info.option = 0;

			printf("[IPC][SEND][SETUP] ch%d, e-time=%04d%02d%02d%02d%02d%02d = %d \n", pSockClient->cmd_info.ch_id, t_end.tm_year+1900, t_end.tm_mon+1, t_end.tm_mday, t_end.tm_hour, t_end.tm_min, t_end.tm_sec, pSockClient->cmd_info.time_end);
			obr_setup(hReader, OBR_EV_PLATE, OBR_IMG_NONE, pSockClient->cmd_info.option);
			usleep(100000);

			printf("[IPC][SEND][READ] ch%d, e-time=%04d%02d%02d%02d%02d%02d = %d \n", pSockClient->cmd_info.ch_id, t_end.tm_year+1900, t_end.tm_mon+1, t_end.tm_mday, t_end.tm_hour, t_end.tm_min, t_end.tm_sec, pSockClient->cmd_info.time_end);

			ev_counter = ev_len = 0;
			while(obr_read(hReader, &result) == 0) 
			{
				ev_counter = ev_counter + 1;

				ev_len = ev_len + strlen(result.plate);
				printf("[IPC][SEND] ch%d recv plate=%s, time=%d, usec=%d \n", pSockClient->cmd_info.ch_id, result.plate, result.sec, result.usec);
				//dbg("RESULT : %s @ %d.%06d\n", result.plate, result.sec, result.usec);
			}
			obr_free(hReader);
		}
		else
			ev_counter = 0;
*/
		ev_counter = ev_counter+1;
		sprintf(log_msg,"[IPC][SEND] [ch%04d]-vsc=%d, recv-event=%d. \n",pSockClient->cmd_info.ch_id, park_list[pSockClient->cmd_info.ch_id-1]+1, ev_counter);
		Write_Log(0,log_msg);

		memset( &(s_cm.h), 0, CMD_H_SZ );
		s_cm.h.dwMagic = CMD_MAGIC;
		s_cm.h.dwVersion = CMD_VERSION;
		//parse msg
		if(ev_counter == 1)
		{
			sprintf(send_msg,"|SendEventList|1|");
			s_cm.h.dwLen = strlen(send_msg) + 25;
		}
		else
		{
			memset(tmp_string, 0 ,32);
			sprintf(tmp_string, "%d", ev_counter);
			s_cm.h.dwLen = (16+ev_len+(ev_counter*25)) + strlen(tmp_string);

			sprintf(send_msg, "|SendEventList|%d|", ev_counter);
		}

		sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", pSockClient->cmd_info.ch_id, send_msg, strlen(send_msg), s_cm.h.dwMagic,  s_cm.h.dwVersion, s_cm.h.dwLen);
		Write_Log(0,log_msg);

		if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ)
		{
			sprintf(log_msg,"[IPC][SEND][ERROR] send event list header(%d) fail ret=%d\n", CMD_H_SZ, ret);
			Write_Log(0,log_msg);
			return -31;
		}
		else if( (ret = send(pSockClient->conn_client, send_msg, strlen(send_msg), 0)) < CMD_H_SZ)
		{
			sprintf(log_msg,"[IPC][SEND][ERROR] send event list send_msg(%d) fail ret=%d\n", strlen(send_msg), ret);
			Write_Log(0,log_msg);
			return -32;
		}

		sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], ready into event list cycle. \n", pSockClient->cmd_info.ch_id);
		Write_Log(0,log_msg);

		if( (strcmp(pSockClient->cmd_info.cOption, "DESC")) == 0) 
		{
			sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], parse event list(counter=%d) .\n", pSockClient->cmd_info.ch_id, ev_counter);
			Write_Log(0,log_msg);
			if(ev_counter > 1)
			{
				for(tmp_int=ev_counter-2 ; tmp_int>-1 ; tmp_int--)
				{
					sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], read ans_list[%d] plate=%s, time=%d, usec=%d. \n", pSockClient->cmd_info.ch_id, tmp_int, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].plate, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].timestamp, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].micro_sec);
					Write_Log(0,log_msg);

					tmp_time_s = ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].timestamp;
					t_tmp_s = localtime(&tmp_time_s);

					sprintf(tmp_string, "CARNO_%04d%02d%02d%02d%02d%02d%03d_%s|", t_tmp_s->tm_year+1900, t_tmp_s->tm_mon+1, t_tmp_s->tm_mday, t_tmp_s->tm_hour, t_tmp_s->tm_min, t_tmp_s->tm_sec, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].micro_sec/1000, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].plate);

					sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], send_msg=%s(%d) \n", pSockClient->cmd_info.ch_id, tmp_string, strlen(tmp_string));
					Write_Log(0,log_msg);

					if( (ret = send(pSockClient->conn_client, tmp_string, strlen(tmp_string), 0)) < strlen(tmp_string))
					{
						sprintf(log_msg,"[IPC][SEND][ERROR] send event lsit %d-msg=%s(%d) error ret=%d\n", ev_dcheck_counter, tmp_string, strlen(tmp_string), ret);
						Write_Log(0,log_msg);
						obr_free(hReader);
						return -32;
					}
				}
			}
			//head empty event
			tmp_time_s = ans_list[pSockClient->cmd_info.ch_id-1].start_timestamp;
			t_tmp_s = localtime(&tmp_time_s);

			sprintf(tmp_string, "CARNO_%04d%02d%02d%02d%02d%02d%03d_|", t_tmp_s->tm_year+1900, t_tmp_s->tm_mon+1, t_tmp_s->tm_mday, t_tmp_s->tm_hour, t_tmp_s->tm_min, t_tmp_s->tm_sec, ans_list[pSockClient->cmd_info.ch_id-1].start_micro_sec/1000);

			sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], send_msg=%s(%d) \n", pSockClient->cmd_info.ch_id, tmp_string, strlen(tmp_string));
			Write_Log(0,log_msg);

			if( (ret = send(pSockClient->conn_client, tmp_string, strlen(tmp_string), 0)) < strlen(tmp_string))
			{
				sprintf(log_msg,"[IPC][SEND][ERROR] send event lsit %d-msg=%s(%d) error ret=%d\n", ev_dcheck_counter, tmp_string, strlen(tmp_string), ret);
				Write_Log(0,log_msg);

				obr_free(hReader);
				return -32;
			}
		}
		else
		{
			for(tmp_int=0 ; tmp_int<ev_counter ; tmp_int++)
			{
				tmp_time_s = ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].timestamp;
				t_tmp_s = localtime(&tmp_time_s);

				sprintf(tmp_string, "CARNO_%04d%02d%02d%02d%02d%02d%03d_%s|", t_tmp_s->tm_year+1900, t_tmp_s->tm_mon+1, t_tmp_s->tm_mday, t_tmp_s->tm_hour, t_tmp_s->tm_min, t_tmp_s->tm_sec, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].micro_sec/1000, ans_list[pSockClient->cmd_info.ch_id-1].answer[tmp_int].plate);

				if( (ret = send(pSockClient->conn_client, tmp_string, strlen(tmp_string), 0)) < strlen(tmp_string))
				{
					sprintf(log_msg,"[IPC][SEND][ERROR] send event lsit %d-msg=%s(%d) error ret=%d\n", ev_dcheck_counter, tmp_string, strlen(tmp_string), ret);
					Write_Log(0,log_msg);
					obr_free(hReader);
					return -32;
				}
			}
		}
/*		if( (hReader = obr_alloc(park_list[pSockClient->cmd_info.ch_id-1], pSockClient->cmd_info.ch_id, pSockClient->cmd_info.time_start, pSockClient->cmd_info.time_end)) != 0)
		{
			if( (strcmp(pSockClient->cmd_info.cOption, "DESC")) == 0) 
				pSockClient->cmd_info.option = 1;
			else
 				pSockClient->cmd_info.option = 0;

			obr_setup(hReader, OBR_EV_PLATE, OBR_IMG_NONE, pSockClient->cmd_info.option);

			ev_dcheck_counter = 0;
			while(obr_read(hReader, &result) == 0) 
			{
				ev_counter = ev_counter -1;
				ev_dcheck_counter = ev_dcheck_counter + 1;

				printf("[IPC][SEND] recv event-%s, time=%d, usec=%d \n", result.plate, result.sec, result.usec);

				gmtime_r(&(result.sec), &t_end);
				sprintf(tmp_string, "CARNO_%04d%02d%02d%02d%02d%02d%03d_%s|", t_end.tm_year+1900, t_end.tm_mon+1, t_end.tm_mday, t_end.tm_hour, t_end.tm_min, t_end.tm_sec, result.usec/1000, result.plate);
				if( (ret = send(pSockClient->conn_client, tmp_string, strlen(tmp_string), 0)) < strlen(tmp_string))
				{
					printf("[IPC][SEND][ERROR] send event lsit %d-msg=%s(%d) error ret=%d\n", ev_dcheck_counter, tmp_string, strlen(tmp_string), ret);
					obr_free(hReader);
					return -32;
				}

				printf("[IPC][SEND] send event lsit %d-msg=%s(%d).\n", ev_dcheck_counter, tmp_string, strlen(tmp_string) );
#ifdef dbg
				sprintf(dbg_sting, "[SEND] send event lsit %d-msg=%s(%d).\n", ev_dcheck_counter, tmp_string, strlen(tmp_string) );
				fwrite(dbg_sting, sizeof(char), strlen(dbg_sting), dbg_file);
#endif
				//strcat(send_msg, tmp_string);
				//sprintf(send_msg, "%sCARNO_%04d%02d%02d%02d%02d%02d%03d_%s|", send_msg, t_end.tm_year+1900, t_end.tm_mon+1, t_end.tm_mday, t_end.tm_hour, t_end.tm_min, t_end.tm_sec, result.usec/1000, result.plate);
				if (ev_counter == 0) //確保在重新讀取的時間中沒有新增事件造成資料混亂
					break;
			}
				
			obr_free(hReader);
		}*/
		ans_list[pSockClient->cmd_info.ch_id-1].list_inuse = 0;
		sprintf(log_msg,"[IPC][SEND] ev-[ch%04d], leave event list cycle.\n", pSockClient->cmd_info.ch_id);
		Write_Log(0,log_msg);
		break;
	case 4://|SendEventData|Len|MetaData|
		pSockClient->cmd_info.ch_id = atoi(pSockClient->cmd_info.cFile+2);
		
		//Check event type
		t = strstr(pSockClient->cmd_info.cEvent, "_");
		*t = 0;
		strcpy(tmp_string, pSockClient->cmd_info.cEvent);
		
		if(strcmp(tmp_string, "CARNO") == 0)
		{
			pSockClient->cmd_info.ev_type = 0;

			t1 = t+1;
			t = strstr(t1, "_");
			*t = 0;
			strcpy(tmp_string, t1);
			pSockClient->cmd_info.time_end = Convert_Time_string(tmp_string);
			
			strcpy(pSockClient->cmd_info.cName, t+1);
		}
		else if ( strcmp(tmp_string, "VLOST") == 0)			
		{
			pSockClient->cmd_info.ev_type = 1;

			strcpy(tmp_string, t+1);
			pSockClient->cmd_info.time_end = Convert_Time_string(tmp_string);
		}
		else if (strcmp(tmp_string, "VRECOVER") == 0)
		{
			pSockClient->cmd_info.ev_type = 2;

			strcpy(tmp_string, t+1);
			pSockClient->cmd_info.time_end = Convert_Time_string(tmp_string);
		}
		else
			return -1;

		//check data type
		if( (strcmp(pSockClient->cmd_info.cData_type, "IMAGE")) == 0) 
			pSockClient->cmd_info.date_type = 0;
		else
			pSockClient->cmd_info.date_type = 1;

		sprintf(log_msg,"[IPC][SEND] ev-chid=%d, ev-time=%ld, ev-type=%d, ev-dtype=%d \n", pSockClient->cmd_info.ch_id, pSockClient->cmd_info.time_end, pSockClient->cmd_info.ev_type, pSockClient->cmd_info.date_type);
		Write_Log(0,log_msg);

		if( (hReader = obr_alloc(park_list[pSockClient->cmd_info.ch_id-1], pSockClient->cmd_info.ch_id, 0, pSockClient->cmd_info.time_end)) != 0)
		{
			obr_setup(hReader, pSockClient->cmd_info.ev_type, pSockClient->cmd_info.date_type, OBR_ORDER_ASCENT);
			//while(obr_read(hReader, &result) == 0) 
			if(obr_read(hReader, &result) == 0)
			{
				sprintf(log_msg,"[IPC][SEND] get event data ,result size=%d \n", result.size);
				Write_Log(0,log_msg);
				memset(tmp_string, 0 ,32);
				sprintf(tmp_string, "%d", result.size);

				s_cm.h.dwLen = 17 + strlen(tmp_string) + result.size;

				//send_msg = (char *)malloc(sizeof(char)*(17+strlen(tmp_string)));
				sprintf(send_msg, "|SendEventData|%d|", result.size);
// 				memcpy(send_msg+16+strlen(tmp_string), result.buffer, result.size);
// 				send_msg[s_cm.h.dwLen] = '|';
// 				send_msg[s_cm.h.dwLen+1] = 0;

				sprintf(log_msg,"[IPC][SEND] send event data msg-len=%d\n", s_cm.h.dwLen);
				Write_Log(0,log_msg);

				if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ)
				{
					sprintf(log_msg,"[IPC][SEND][ERROR]  sned event data header(%d) fail ret=%d\n", CMD_H_SZ, ret);
					Write_Log(0,log_msg);
					obr_free(hReader);
					return -41;
				}
				else if( (ret = send(pSockClient->conn_client, send_msg, strlen(send_msg), 0)) < strlen(send_msg))
				{
					sprintf(log_msg,"[IPC][SEND][ERROR]  sned event data send_msg(%d) fai ret=%d\n", strlen(send_msg), ret);
					Write_Log(0,log_msg);
					obr_free(hReader);
					return -42;
				}
				else if( (ret = send(pSockClient->conn_client, result.buffer, result.size, 0)) < result.size)
				{
					sprintf(log_msg,"[IPC][SEND][ERROR] sned event data result(%d) fail ret=%d\n", result.size, ret);
					Write_Log(0,log_msg);
					obr_free(hReader);
					return -43;
				}
				else if( (ret = send(pSockClient->conn_client, "|", 1, 0)) < 1)
				{
					sprintf(log_msg,"[IPC][SEND][ERROR] sned event data |(1) fail ret=%d\n", ret);
					Write_Log(0,log_msg);
					obr_free(hReader);
					return -44;
				}
			}
			obr_free(hReader);
		}
		break;
	case 5://|SendVideoData|Len|MetaData|

		pSockClient->cmd_info.ev_type = 3;

		t = strstr(pSockClient->cmd_info.cFile, "-");
		*t = 0;
		tmp_int = atoi(t+2);
		pSockClient->cmd_info.ch_id = tmp_int-1;
		char		tmp_string[32];
		char		time_string[32];
		char		tmp_msg[32];
		int			start,end,cut_flag,cflag,ix,start_snap,end_snap;
		int			totalframe;
		int			nowframe;
		int			c = 0;
		int			v5_id;
		v5_id = park_list[pSockClient->cmd_info.ch_id];
		nowframe = 0;
		totalframe = 0;
		cut_flag =0;
		cflag = 0;
		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time+8, 2);
		start = ( atoi(tmp_string));
		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time+8, 2);
		end = ( atoi(tmp_string));
		if ((start - end)!=0)
		{
			cut_flag = 1;
			cflag	 = 1;
		}
		//FILE *fp;
		//char s[512]={0};
		//sprintf(tmp_msg,"//dbg//vs//%d.ovs",pSockClient->cmd_info.ch_id);
		//fp = fopen(tmp_msg, "a+");
		//if(fp)
		//{
		//	fwrite(s, 1 , 512 , fp);
		//	fclose(fp);
		//}
		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cStrat_time+8, 6);
		start_snap =atoi(tmp_string);
		memset(tmp_string, 0 ,32);
		strncpy(tmp_string, pSockClient->cmd_info.cEnd_time+8, 6);
		end_snap =atoi(tmp_string);
		if (start_snap == end_snap){
			jsize = (pSockClient->reconfig->ch_info[v5_id].rec->heigh) * (pSockClient->reconfig->ch_info[v5_id].rec->width);
			for(ix=0;ix<5;ix++)
			{
				sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] While buf_state = %d\n",pSockClient->reconfig->ch_info[v5_id].rec->buf_state);
				Write_Log(0,log_msg);
				if(pSockClient->reconfig->ch_info[v5_id].rec->buf_state ==0)
				{
					pSockClient->reconfig->ch_info[v5_id].rec->buf_state = 1;
					memset(jbuf,0,5000000);
					Write_Log(0,"[IPC] [Prepare_Msg] [SEND] [CASE5] Inside buf_state \n");
					if (jsize <0)
					{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] jsize = %d\n",jsize);
						Write_Log(0,log_msg);
						return -40;
					}

					aft.magic = 0x6153664d;
					aft.width = pSockClient->reconfig->ch_info[v5_id].rec->width;
					aft.height = pSockClient->reconfig->ch_info[v5_id].rec->heigh;
					aft.size = pSockClient->reconfig->ch_info[v5_id].rec->width*pSockClient->reconfig->ch_info[v5_id].rec->heigh;
					aft.sec = 0;
					aft.usec = 0;
					aft.type = 1;
					aft.sub_type = 17;

					sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] Prepare send image\n");
					Write_Log(0,log_msg);
					memset(tmp_string, 0 ,32);
					sprintf(tmp_string, "%d", jsize);
					sprintf(send_msg, "|SendVideoData|1|0|%d|", jsize);
					s_cm.h.dwLen = 1 + strlen(send_msg) + jsize + sizeof(as_frame_t);
					//}
					sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] ev-[ch%04d] snap - jsize=%d, send_msg=%s, le=%d, width = %d, height = %d. \n", v5_id, jsize, send_msg, s_cm.h.dwLen,aft.width,aft.height);
					Write_Log(0,log_msg);

					if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ)
					{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send snapshot heander(%d) fail ret=%d\n", CMD_H_SZ, ret);
						Write_Log(0,log_msg);
						pSockClient->reconfig->ch_info[v5_id].rec->buf_state = 0;
						return -41;
					}

					if( (ret = send(pSockClient->conn_client, send_msg, strlen(send_msg), 0)) < strlen(send_msg))
					{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send snapshot send_msg(%d) fail ret=%d\n", strlen(send_msg), ret);
						Write_Log(0,log_msg);
						pSockClient->reconfig->ch_info[v5_id].rec->buf_state = 0;
						return -42;
					}


					if( (ret = send(pSockClient->conn_client, (char *)&aft , sizeof(as_frame_t), 0)) < sizeof(as_frame_t))
					{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send video header fail ret=%d\n", ret);
						Write_Log(0,log_msg);
						obr_free(hReader);
						return -43;
					}

					if((ret = send(pSockClient->conn_client, pSockClient->reconfig->ch_info[v5_id].rec->buf_snap, jsize, 0)) < jsize)
					{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send snapshot jbuf(%d) fail ret=%d\n", ret, jsize);
						Write_Log(0,log_msg);
						pSockClient->reconfig->ch_info[v5_id].rec->buf_state = 0;
						return -44;
					}
					//}

					if( (ret = send(pSockClient->conn_client, "|", 1, 0)) < 1)
					{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send snapshot |(1) fail ret=%d\n", ret);
						Write_Log(0,log_msg);
						pSockClient->reconfig->ch_info[v5_id].rec->buf_state = 0;
						return -45;
					}
					pSockClient->reconfig->ch_info[v5_id].rec->buf_state = 0;
					break;
				}
				else if (ix == 4)
				{
					pSockClient->reconfig->ch_info[v5_id].rec->buf_state = 0;
					return -46;
				}
				usleep(10000);
			}
		}
		else{
			for(ix=cut_flag;ix>=0;ix--) 
			{
				memset(time_string, 0 ,32);
				if( strcmp(pSockClient->cmd_info.cStrat_time, "") == 0)
					pSockClient->cmd_info.time_start = 0;
				else if (ix == 0 && cflag == 1)
				{
					strncpy(time_string,pSockClient->cmd_info.cEnd_time,10);
					strcat(time_string,"0000000");
					pSockClient->cmd_info.time_start = Convert_Time_string(time_string);
				}
				else 
					pSockClient->cmd_info.time_start = Convert_Time_string(pSockClient->cmd_info.cStrat_time);
			

				if( strcmp(pSockClient->cmd_info.cEnd_time, "") == 0)
					pSockClient->cmd_info.time_end = 0;
				else if (ix == 1 && cflag == 1)
				{
					strncpy(time_string,pSockClient->cmd_info.cStrat_time,10);
					strcat(time_string,"5959000");
					pSockClient->cmd_info.time_end = Convert_Time_string(time_string);
				}
				else 
					pSockClient->cmd_info.time_end = Convert_Time_string(pSockClient->cmd_info.cEnd_time);

				if( (hReader = obr_alloc(park_list[pSockClient->cmd_info.ch_id], pSockClient->cmd_info.ch_id, pSockClient->cmd_info.time_start, pSockClient->cmd_info.time_end)) != 0)
				{
					obr_setup(hReader, pSockClient->cmd_info.ev_type, OBR_IMG_NONE, OBR_ORDER_ASCENT);
				
					while(obr_read(hReader, &result) == 0) 
					{
						totalframe++;
					}
					obr_free(hReader);
				}	
				sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] send video total frame = %d start = %d end = %d\n", totalframe, pSockClient->cmd_info.time_start, pSockClient->cmd_info.time_end);
				Write_Log(0,log_msg);
			} 
			//Send Msg
			do 
			{
				memset(time_string, 0 ,32);
				if( strcmp(pSockClient->cmd_info.cStrat_time, "") == 0)
					pSockClient->cmd_info.time_start = 0;
				else if (cut_flag == 0 && cflag == 1)
				{
					strncpy(time_string,pSockClient->cmd_info.cEnd_time,10);
					strcat(time_string,"0000000");
					pSockClient->cmd_info.time_start = Convert_Time_string(time_string);
				}
				else 
					pSockClient->cmd_info.time_start = Convert_Time_string(pSockClient->cmd_info.cStrat_time);

				if( strcmp(pSockClient->cmd_info.cEnd_time, "") == 0)
					pSockClient->cmd_info.time_end = 0;
				else if (cut_flag == 1 && cflag == 1)
				{
					strncpy(time_string,pSockClient->cmd_info.cStrat_time,10);
					strcat(time_string,"5959000");
					pSockClient->cmd_info.time_end = Convert_Time_string(time_string);
				}
				else 
					pSockClient->cmd_info.time_end = Convert_Time_string(pSockClient->cmd_info.cEnd_time);

				sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] send video start time %d end time %d. \n",pSockClient->cmd_info.time_start,pSockClient->cmd_info.time_end);
				Write_Log(0,log_msg);
				sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] park_list = %d cmd_info = %d. \n",park_list[pSockClient->cmd_info.ch_id],pSockClient->cmd_info.ch_id);
				Write_Log(0,log_msg);

				if( (hReader = obr_alloc(park_list[pSockClient->cmd_info.ch_id], pSockClient->cmd_info.ch_id, pSockClient->cmd_info.time_start, pSockClient->cmd_info.time_end)) != 0)
				{
					obr_setup(hReader, pSockClient->cmd_info.ev_type, OBR_IMG_NONE, OBR_ORDER_ASCENT);
					while(obr_read(hReader, &result) == 0) 
						{
							memset(tmp_string, 0 ,32);
							sprintf(tmp_string, "%d", result.size);
							aft.magic = 0x6153664d;
							aft.width = result.width;
							aft.height = result.height;
							aft.size = result.size;
							aft.sec = result.sec;
							aft.usec = result.usec;
							aft.type = 1;
							aft.sub_type = 17;			
							//Write_Log(0,log_msg);

							/*sprintf(log_msg,"[IPC][SEND][CASE 5] send video data msg-len=%d , buffer size = %d. \n", s_cm.h.dwLen , result.size);
							Write_Log(0,log_msg);*/
							sprintf(send_msg, "|SendVideoData|%d|%d|%d|",totalframe,nowframe,result.size);
							nowframe++;
							sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] send_msg = %s totalframe = %d , nowframe = %d result.size = %d\n",send_msg,totalframe,nowframe , result.size);
							Write_Log(0,log_msg);
							s_cm.h.dwLen = 1 + strlen(send_msg) + result.size + sizeof(as_frame_t);
							//sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE5] totalLen = %d msg = %d result.size = %d as_frame_t = %d aft_len = %d \n",s_cm.h.dwLen,strlen(send_msg),result.size,sizeof(as_frame_t),aft.size);
							//Write_Log(0,log_msg);

							//char path[256];
							//if(c == 0)
							//{
							//	sprintf(path, "//dbg//record.obi");
							//	RAW2File(path,result.buffer,1920,1080,8,IMG_FMT_OBI,0);	
							//	c++;
							//}


							if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ)
							{
								sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send video header(%d) fail ret=%d\n", CMD_H_SZ, ret);
								Write_Log(0,log_msg);
								obr_free(hReader);
								return -51;
							}
							else if( (ret = send(pSockClient->conn_client, send_msg, strlen(send_msg), 0)) < strlen(send_msg))
							{
								sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send video data(%d) fail ret=%d\n", s_cm.h.dwLen, ret);
								Write_Log(0,log_msg);
								obr_free(hReader);
								return -52;
							}
							else if( (ret = send(pSockClient->conn_client, (char *)&aft , sizeof(as_frame_t), 0)) < sizeof(as_frame_t))
							{
								sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send video header fail ret=%d\n", ret);
								Write_Log(0,log_msg);
								obr_free(hReader);
								return -53;
							}
							else if( (ret = send(pSockClient->conn_client, result.buffer, result.size, 0)) < result.size)
							{
								sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send video buffer fail ret=%d\n", ret);
								Write_Log(0,log_msg);
								obr_free(hReader);
								return -54;
							}
							else if( (ret = send(pSockClient->conn_client, "|" , 1, 0)) < 1)
							{
								sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE5] send '|' ret=%d\n", ret);
								Write_Log(0,log_msg);
								obr_free(hReader);
								return -55;
							}
							//fp = fopen(tmp_msg, "a+");
							//if(fp)
							//{
							//	fwrite((char *)&aft , 1 , sizeof(as_frame_t) , fp);
							//	fclose(fp);
							//}

							//			fp = fopen(tmp_msg, "a+");
							//			if(fp)
							//			{
							//				fwrite(result.buffer, 1, result.size, fp);
							//				fclose(fp);
							//			}
							//free(send_msg);
						}
					obr_free(hReader);
				}			
				if (cut_flag ==0)
				{
					break;
				}
				else
				{
					cut_flag--;
				}
			} while (1);
		}
		break;
	case 6://|SendSnapshot|Len|MetaData|
		now = (u32) time(0);
		int psize;
		int v_id;
		char ch_vs[32];
		v_id = park_list[pSockClient->cmd_info.ch_id-1];
		//memset(tbuf,0,5000000);
		//memcpy(tbuf,pSockClient->reconfig->ch_info[v_id].rec->buf_snap,pSockClient->reconfig->ch_info[v_id].rec->heigh*pSockClient->reconfig->ch_info[v_id].rec->width);
		if(pSockClient->reconfig->ch_info[v_id].rec->buf_snap) 
		{
			tmp_time_e = now;
			t_tmp_e = localtime(&tmp_time_e);
			jsize = (pSockClient->reconfig->ch_info[v_id].rec->heigh) * (pSockClient->reconfig->ch_info[v_id].rec->width);
			sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE6] Buffer is not empty jsize = %d , ch_id = %d\n",jsize,pSockClient->cmd_info.ch_id-1);
			Write_Log(0,log_msg);
			int ix;
			/*for (ix=0;ix<16;ix++)
			{
				sprintf(log_msg,"[TEST] %d ch_id %d\n",ix,park_list[ix]);
				Write_Log(0,log_msg);
			}*/
			//memcpy(jbuf,pSockClient->reconfig->ch_info[pSockClient->cmd_info.ch_id-1].rec->buf_snap,jsize); 
			for(ix=0;ix<5;ix++)
			{
				sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE6] While buf_state = %d\n",pSockClient->reconfig->ch_info[v_id].rec->buf_state);
				Write_Log(0,log_msg);
				if(pSockClient->reconfig->ch_info[v_id].rec->buf_state ==0)
				{
					pSockClient->reconfig->ch_info[v_id].rec->buf_state = 1;
					memset(jbuf,0,5000000);

					if (pSockClient->reconfig->ch_info[v_id].rec->heigh == 1080)
					{
						Write_Log(0,"[CODEC] 1080\n");
						if(ans_list[pSockClient->cmd_info.ch_id-1].tag == 0)
							psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), pSockClient->reconfig->ch_info[v_id].rec->width/4, 8, (pSockClient->reconfig->ch_info[v_id].rec->width/2),pSockClient->reconfig->ch_info[v_id].rec->heigh-8,0,0);
						if(ans_list[pSockClient->cmd_info.ch_id-1].tag == 1)
							psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), 0, 8, (pSockClient->reconfig->ch_info[v_id].rec->width/2),pSockClient->reconfig->ch_info[v_id].rec->heigh-8,0,0);
						else if (ans_list[pSockClient->cmd_info.ch_id-1].tag == 2)
							psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), (pSockClient->reconfig->ch_info[v_id].rec->width/2), 8, (pSockClient->reconfig->ch_info[v_id].rec->width/2), pSockClient->reconfig->ch_info[v_id].rec->heigh-8,0,0);
					}
					else if(pSockClient->reconfig->ch_info[v_id].rec->heigh == 540)
					 {
						 Write_Log(0,"[CODEC] 540\n");
						 if(ans_list[pSockClient->cmd_info.ch_id-1].tag == 0)	
							 psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), pSockClient->reconfig->ch_info[v_id].rec->width/4, 8, (pSockClient->reconfig->ch_info[v_id].rec->width/2),pSockClient->reconfig->ch_info[v_id].rec->heigh-12,0,0);
						 if(ans_list[pSockClient->cmd_info.ch_id-1].tag == 1)
							 psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), 0, 8, (pSockClient->reconfig->ch_info[v_id].rec->width/2),pSockClient->reconfig->ch_info[v_id].rec->heigh-12,0,0);
						 else if (ans_list[pSockClient->cmd_info.ch_id-1].tag == 2)
							 psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), (pSockClient->reconfig->ch_info[v_id].rec->width/2), 8, (pSockClient->reconfig->ch_info[v_id].rec->width/2), pSockClient->reconfig->ch_info[v_id].rec->heigh-12,0,0);
					}
					else 
					{
						Write_Log(0,"[CODEC] other\n");
						if(ans_list[pSockClient->cmd_info.ch_id-1].tag == 0)	
							psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, jsize , jbuf, jsize, 0, 0, pSockClient->reconfig->ch_info[v_id].rec->width, pSockClient->reconfig->ch_info[v_id].rec->heigh,0,0);
						if(ans_list[pSockClient->cmd_info.ch_id-1].tag == 1)
							psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), 0, 0, (pSockClient->reconfig->ch_info[v_id].rec->width/2), pSockClient->reconfig->ch_info[v_id].rec->heigh,0,0);
						else if (ans_list[pSockClient->cmd_info.ch_id-1].tag == 2)
							psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, (jsize/2) , jbuf, (jsize/2), (pSockClient->reconfig->ch_info[v_id].rec->width/2), 0, (pSockClient->reconfig->ch_info[v_id].rec->width/2), pSockClient->reconfig->ch_info[v_id].rec->heigh,0,0);
					}
					
					Write_Log(0,"[IPC] [Prepare_Msg] [SEND] [CASE6] Inside buf_state \n");
					//fp = fopen("file.h64", "rb");
					//if(fp)
					//{
					//	fwrite(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, 1, jsize, fp);
					//	fclose(fp);
					//}
					//psize = h264tojpeg(pSockClient->reconfig->ch_info[v_id].rec->buf_snap, jsize , jbuf, jsize, 0, 0, 1920, 480);
					
					sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE6] After write image %d width = %d height = %d\n",psize,pSockClient->reconfig->ch_info[v_id].rec->width,pSockClient->reconfig->ch_info[v_id].rec->heigh);
					Write_Log(0,log_msg);
					jsize = psize;
					//sprintf(ch_vs,"%d-vs%d.jpg",pSockClient->cmd_info.ch_id,v_id);
					//fp = fopen("file_vs2.jpg", "wb");
					//if(fp)
					//{
					//	fwrite(jbuf, 1, psize, fp);
					//	fclose(fp);
					//}
					//sprintf(log_msg,"[IPC][CASE 6] Before write image\n");
					//Write_Log(0,log_msg);
					if (jsize <0)
					{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE6] psize = %d\n",psize);
						Write_Log(0,log_msg);
						return -60;
					}
					
					sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE6] Prepare send image\n");
					Write_Log(0,log_msg);
					memset(tmp_string, 0 ,32);
					sprintf(tmp_string, "%d", jsize);
					s_cm.h.dwLen = 16 + strlen(tmp_string) + jsize;
					sprintf(send_msg, "|SendSnapshot|%d|", jsize);
				//}
				sprintf(log_msg,"[IPC] [Prepare_Msg] [SEND] [CASE6] ev-[ch%04d] snap - jsize=%d, send_msg=%s, le=%d. \n", v_id, jsize, send_msg, s_cm.h.dwLen);
				Write_Log(0,log_msg);

				if( (ret = send(pSockClient->conn_client, (char *)&s_cm, CMD_H_SZ, 0)) < CMD_H_SZ)
				{
					sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE6] send snapshot heander(%d) fail ret=%d\n", CMD_H_SZ, ret);
					Write_Log(0,log_msg);
					pSockClient->reconfig->ch_info[v_id].rec->buf_state = 0;
					return -61;
				}
			
				if( (ret = send(pSockClient->conn_client, send_msg, strlen(send_msg), 0)) < strlen(send_msg))
				{
					sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE6] send snapshot send_msg(%d) fail ret=%d\n", strlen(send_msg), ret);
					Write_Log(0,log_msg);
					pSockClient->reconfig->ch_info[v_id].rec->buf_state = 0;
					return -62;
				}


				if((ret = send(pSockClient->conn_client, jbuf, jsize, 0)) < jsize)
				{
						sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE6] send snapshot jbuf(%d) fail ret=%d\n", ret, jsize);
						Write_Log(0,log_msg);
						pSockClient->reconfig->ch_info[v_id].rec->buf_state = 0;
						return -63;
				}
				//}
			
				if( (ret = send(pSockClient->conn_client, "|", 1, 0)) < 1)
				{
					sprintf(log_msg,"[IPC] [Prepare_Msg] [SENDE] [CASE6] send snapshot |(1) fail ret=%d\n", ret);
					Write_Log(0,log_msg);
					pSockClient->reconfig->ch_info[v_id].rec->buf_state = 0;
					return -64;
				}
				//psize = h264tojpeg(tbuf, (jsize/2) , jbuf, (jsize/2), pSockClient->reconfig->ch_info[v_id].rec->width/4, 8, (pSockClient->reconfig->ch_info[v_id].rec->width/2),pSockClient->reconfig->ch_info[v_id].rec->heigh-8);

				pSockClient->reconfig->ch_info[v_id].rec->buf_state = 0;
				break;
				}
				else if (ix == 4)
				{
					pSockClient->reconfig->ch_info[v_id].rec->buf_state = 0;
					return -65;
				}
				usleep(10000);
			}
		}

		break;
	default://Error cmd_id
		break;
	}
	return 1;
}

void	Connected(void *arg)
{
	socket_client	*pClient = (socket_client*) arg;
	char			log_msg[MSG_SIZE];

	int				ret;

	//protocol struct
	char			*buf = (char *)malloc(sizeof(char)*1024);
	CMD_H			*fh;
	//DWORD			magic_be;

	//select members
	fd_set			rds;
	struct timeval	tv;

	//pClient->msg = (char *)malloc(sizeof(char)*1024);
	
	sprintf(log_msg,"[IPC] [Connected] [CMD] client = %d, work flag = %d\n", pClient->conn_client, pClient->worked);
	Write_Log(1,log_msg);

	while(pClient->worked)
	{		
		FD_ZERO(&rds);
		FD_SET(pClient->conn_client, &rds);

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		ret = select(pClient->conn_client+1, &rds, 0, 0, &tv);

		if(ret < 0)
		{
			if(errno != EINTR)
			{
				break;
			}
			continue;
		}
		else if(ret == 0 || !FD_ISSET(pClient->conn_client, &rds))
			continue;
		
		sprintf(log_msg,"[IPC] [Connected] [CMD] client = %d, start recv header\n", pClient->conn_client);
		Write_Log(1,log_msg);
		//read header
		memset(buf,0,1024);
		fh = (CMD_H *)buf;
        ret = recv(pClient->conn_client, buf, CMD_H_SZ, 0);
		sprintf(log_msg,"[IPC] [Connected] [CMD] [RECV] client = %d,  %x version = %x len = %x\n",pClient->conn_client, fh->dwMagic,fh->dwVersion,fh->dwLen);
		Write_Log(0,log_msg);
        if ( (ret != CMD_H_SZ) || (fh->dwMagic != CMD_MAGIC) ) 
		{
			if(ret < 0) //連接被重設(reset)，則呼叫失敗
			{
				sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d, recv header fail \n",pClient->conn_client);
				Write_Log(0,log_msg);
				goto Connection_exit;
			}
            else if(ret == 0)	//!彼端連接中斷or停止傳送資料!
			{
				sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d, recv header stop \n",pClient->conn_client);
				Write_Log(0,log_msg);
				goto Connection_exit;
			}
			else if(ret < CMD_H_SZ) //接收到的資料長度不對
			{
				sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d, header len=%d \n",pClient->conn_client, ret);
				Write_Log(0,log_msg);
				goto Connection_exit;
			}
			else //接收到的magic有問題
			{
				sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d, wrong magic %x != %x version = %x len = %x\n",pClient->conn_client, fh->dwMagic, CMD_MAGIC,fh->dwVersion,fh->dwLen);
				Write_Log(0,log_msg);
				goto Connection_exit;
			}

			/*/find next head
            magic_be = htonl(CMD_MAGIC);
            while (magic != magic_be) 
			{
                if (off >= ret) {
                    off = 0;
                    ret = recv((SOCKET)serverConn, (char *)rbuf, 1024, 0);
                    if(ret <= 0) {
						srv_err("[%d] recv failed-b: %s", s->id, strerror(errno));			
						sprintf(Msg, "[%d] recv failed-b: %s", s->id, strerror(errno));				//del 1013
						WriteLog(hSrvLog, 0, Msg);													//del 1013	
						
						goto _srv_exit;
                    }
                }
                magic = (magic << 8) | rbuf[off];
                off++;
                if (total++ > (1024*2048)) {
					srv_err("[%d] magic not found, recover failed!", s->id);
					sprintf(Msg,"[%d] magic not found, recover failed!", s->id);					//del 1013 
					WriteLog(hSrvLog, 0, Msg);														//del 1013
                    
					goto _srv_exit;
                }
            } 

            if (off < ret)
                memcpy(rbuf+4, &rbuf[off], ret-off);
            ret = ret - off + 4;
            if (ret < CMD_H_SZ) {
                int nret = recv((SOCKET)serverConn, (char *)rbuf+ret, CMD_H_SZ-ret, 0);
                if (nret != (CMD_H_SZ-ret)) {
					srv_err("[%d] recv faild-c: %s", s->id, strerror(errno));
					sprintf(Msg,"[%d] recv faild-c: %s", s->id, strerror(errno));					//del 1013 
					WriteLog(hSrvLog, 0, Msg);														//del 1013
                    
					goto _srv_exit;
                }
                ret = CMD_H_SZ;
            }*/
        }

		//read message
		FD_ZERO(&rds);
		FD_SET(pClient->conn_client, &rds);

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		ret = select(pClient->conn_client+1, &rds, 0, 0, &tv);
		//do 
		//{
		//	ret = select(pClient->conn_client+1, &rds, 0, 0, &tv);
		//	if(ret < 0)
		//	{
		//		if(errno != EINTR)
		//		{
		//			goto Connection_exit;
		//		}
		//		sprintf(log_msg,"[IPC] [CMD] [ERROR] %s\n",strerror(errno));
		//		Write_Log(0,log_msg);

		//	}
		//	else if(ret == 0 || !FD_ISSET(pClient->conn_client, &rds))
		//	{
		//		sprintf(log_msg,"[IPC] [CMD] [ERROR] select = 0\n");
		//		Write_Log(0,log_msg);				
		//	}
		//	count++;
		//	usleep(100);
		//	if (count == 5)
		//	{
		//		goto Connection_exit;
		//	}
		//}while(1)
		if(ret < 0)
		{
			if(errno != EINTR)
			{
				break;
			}
			sprintf(log_msg,"[IPC] [Connected] [CMDE] %s\n",strerror(errno));
			Write_Log(0,log_msg);
			continue;
		}
		else if(ret == 0 || !FD_ISSET(pClient->conn_client, &rds))
		{
			sprintf(log_msg,"[IPC] [Connected] [CMDE] select = 0\n");
			Write_Log(0,log_msg);		
			continue;
		}
		memset(pClient->msg, 0, 1024 );
		ret = recv(pClient->conn_client, pClient->msg, fh->dwLen, 0);
		if(ret <=0)
		{//receive fail
			if (ret == 0)
			{
				sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d, get value_len=%d ,recv command len=0 \n",pClient->conn_client, ret);
				Write_Log(0,log_msg);
			}
			goto Connection_exit;
			break;
		}
		else if(ret < fh->dwLen)
		{
			sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d,get value_len=%d, msg= %s ,recv command len fail\n", pClient->conn_client, ret, pClient->msg);			
			Write_Log(0,log_msg);
			//sleep(50);
		}
		else
		{
			memset(&pClient->cmd_info, 0, sizeof(cmd_option));
			sprintf(log_msg,"[IPC] [Connected] [CMD]  recv msg=%s \n", pClient->msg);
 			Write_Log(0,log_msg);
			if( (ret = Command_Analysis(pClient)) <= 0 )
			{
				sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d, cmd analysis=%d \n", pClient->conn_client, pClient->cmd_info.cmd_id);
				Write_Log(0,log_msg);
				goto Connection_exit;
				break;
			}
  			else if( (ret=Prepare_Msg(pClient)) <= 0 )
 			{
 				sprintf(log_msg,"[IPC] [Connected] [CMDE] client = %d, send back=%d, fail \n", pClient->conn_client, ret);
				Write_Log(0,log_msg);
				goto Connection_exit;
 				break;
 			}
 			else{
 				sprintf(log_msg,"[IPC] [Connected] [CMD] conn=%d, send over\n", pClient->conn_client);
				Write_Log(1,log_msg);
				}
			break;
		}
	}

Connection_exit:
	//socket client error or close
	close(pClient->conn_client);
	free(buf);

	pClient->conn_client = -1;

	pClient->worked = -1;
	sprintf(log_msg,"[IPC] [Connected] [CMD] close client = %d, work flag = %d\n", pClient->conn_client, pClient->worked);
	Write_Log(1,log_msg);
}

void	Start_Listen(void *arg)
{//sample : http://www.binarytides.com/server-client-example-c-sockets-linux/
	socket_server		*pServer = (socket_server*) arg;
	
	int					x, connRecod, yes;
	DWORD				dwThreadId;
	struct sockaddr_in	serv_addr, addrClient;
	int					len = sizeof(struct sockaddr_in);
	int					tmp_connRecod;
	char				log_msg[MSG_SIZE];

	if(pServer->hSockSrv != NULL)
		close(pServer->hSockSrv);

	pServer->hSockSrv = socket(AF_INET,SOCK_STREAM,0); 

	serv_addr.sin_family=AF_INET;
	//serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port=htons(pServer->port_num);

	yes = 1;
	if ( setsockopt(pServer->hSockSrv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
	{
		//perror();
		Write_Log(0,"[IPC] [Start_Listen] setsockopt");
	}

	bind(pServer->hSockSrv,(struct sockaddr*)&serv_addr,sizeof(serv_addr)); 

	for (x=0 ; x<SOCK_NO ; x++)
	{
		pServer->client[x].conn_client = -1;
		pServer->client[x].worked = -1;
	}
	connRecod = 0;

	if( (x=listen(pServer->hSockSrv,5)) != 0)
	{
		return;
	}
	pServer->lis_confirm = 1;
	//printf();
	Write_Log(0,"[IPC] [Start_Listen] Start Listening. \n");
	
	//listen(hSockSrv,SOMAXCONN);
	//listen(hSockSrv,somaxconn);
	for (x=0;x<MAX_CH;x++){	temp_ans[x].send_flag=0;memset(temp_ans[x].ans, 0 ,sizeof(result_info)*MAX_CH);}
	while(1)
	{
		pServer->client[connRecod].conn_client=accept(pServer->hSockSrv, (struct sockaddr*)&addrClient, (socklen_t)&len); 

		if(pServer->client[connRecod].conn_client < 0)
		{// accept fail
			continue;
		}
		pServer->client[connRecod].worked = 1;

		sprintf(log_msg,"[IPC] [Start_Listen] [SEND] accept client = %d, work flag = %d, index=%d. \n", pServer->client[connRecod].conn_client, pServer->client[connRecod].worked, connRecod);
		Write_Log(1,log_msg);

		if (create_new_thread(Connected, &(pServer->client[connRecod])) < 0)
		{
			Write_Log(0,"[IPC] [Start_Listen] [SEND] create thread fail. \n");
			close(pServer->client[connRecod].conn_client);
			continue;
			//break;
		}
		else
		{//check connection
			tmp_connRecod = connRecod;
			//while(connRecod == tmp_connRecod)
			//{
				//printf("[IPC] check free connection. \n");
				Write_Log(0,"[IPC] [Start_Listen] [SEND] check free connection. \n");
				for (x=0 ; x<SOCK_NO ; x++)
				{
					if(pServer->client[x].worked == -1)
					{
						connRecod = x;
						break;
					}
				}
			//}
			if(connRecod == tmp_connRecod)
			{
				for (x=0 ; x<SOCK_NO ; x++)
				{
					pServer->client[x].worked = 0;

					while(pServer->client[x].worked != -1)
					{
						sprintf(log_msg,"[IPC] [Start_Listen] [SENDE] waiting free conntion-%d. \n", x);
						Write_Log(0,log_msg);
						sleep(1);
					}
				}
			}

			/*if(x == SOCK_NO)
			{//need check all connection
				int error = 0;
				socklen_t len = sizeof (error);
				int retval;

				for (x=0 ; x<SOCK_NO ; x++)
				{				
					if ( retval = getsockopt (pServer->client[x].conn_client, SOL_SOCKET, SO_ERROR, &error, &len )  != 0)
					{//conntion have error
						printf("[ERROR][SOCK] connection-%d, was broken-%d. \n", x, retval);
						pServer->client[x].worked = 0;

						while(pServer->client[x].worked != -1)
						{
							printf("[ERROR][SOCK] waiting free conntion-%d. \n", x);
							Sleep(1);
						}
						connRecod = x;
						break;
					}
				}
			}*/
			
			sprintf(log_msg,"[IPC] [Start_Listen] [SEND] accept over index=%d\n", connRecod);
			Write_Log(1,log_msg);
		}
	}

	pServer->lis_confirm = 0;
}


void*	InitSocket(int port, sock_callback_t sockback, int option , rec_config_t *rc)
{
	//DWORD			dwThreadId;
	//socket_server	*hServer = (socket_server *)malloc(sizeof(socket_server)+BSIZE);
	socket_server	*hServer = (socket_server *)malloc(sizeof(socket_server));
	char			log_msg[MSG_SIZE];
	

	hServer->port_num = port;
	hServer->lis_confirm = 0;

	//hServer->img_buf = ((BYTE *)hServer) + sizeof(socket_server);
	//memset(hServer->img_buf , 0 ,BSIZE);

	int	i;
	for (i=0 ; i<SOCK_NO ; i++)
	{
		hServer->client[i].sockback = sockback;
		hServer->client[i].img_type = option;
		//hServer->client[i].plate = hServer->plate;
		//hServer->client[i].img = hServer->img_buf;
		hServer->client[i].sockback = sockback;
		hServer->client[i].worked = -1;
		hServer->client[i].reconfig = rc;
		//hServer->client[i].msg = sock_data + (i*100000);
	}

#ifdef dbg
	dbg_file = fopen("//mnt//test//t.txt");
#endif
	//memset(park_list, -1, 32);

// 	if (create_new_thread(Start_Listen, hServer) < 0)
// 	{
// 		return -1;
// 	}
// 	else
// 	{
// 		return hServer;
// 	}

	return hServer;
}

int	sock_start_listening(void* hSocket_server)
{
	if (create_new_thread(Start_Listen, hSocket_server) < 0)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int	DestroySocket(void *hSocket_server)
{
	socket_server	*hServer = (socket_server *)hSocket_server;

	int		x, y;

	for (x=0 ; x<SOCK_NO ; x++)
	{
		if (hServer->client[x].worked == 1)
		{
			hServer->client[x].worked = 0;
		}
	}

	while(1)
	{
		y = 0;
		for (x=0 ; x<SOCK_NO ; x++)
		{
			if (hServer->client[x].worked == -1)
				y++;

			if(y == 10)
				break;
		}
		sleep(1);
	}
	close(hServer->hSockSrv);

	//printf();
	Write_Log(0,"[IPC] all closed. \n");
}

int	Insert_Answer_List(int parking_id, char* plate, u32 timestamp, u32 micro_sec)
{
	int x = 0;
	char		log_msg[MSG_SIZE];
	//pthread_mutex_lock(&ans_list[parking_id-1].ans_list_mutex);
	while(ans_list[parking_id-1].list_inuse == 1)
	{
		//pthread_mutex_unlock(&ans_list[parking_id-1].ans_list_mutex);
		//return -1;
		usleep(50000);
	}
	ans_list[parking_id-1].list_inuse = 1;
	//pthread_mutex_unlock(&ans_list[parking_id-1].ans_list_mutex);
	
	sprintf(log_msg,"[IPC] [Insert] [ch%04d] - plate:%s , timestamp=%d, usec=%d. \n", parking_id, plate, timestamp, micro_sec);
	Write_Log(0,log_msg);

	//strncpy(hServer->plate+((parking_id-1)*32), plate, 32);
	for (x=0 ; x<32 ; x++)
	{
		if( strcmp(ans_list[parking_id-1].answer[x].plate, plate) == 0 )
		{
			sprintf(log_msg,"[IPC] [Insert] park_id=%d, plate=%s, already in ipc list .\n", parking_id, plate);
			Write_Log(0,log_msg);
			break;
		}

		if( ans_list[parking_id-1].answer[x].timestamp == 0 )
		{
			strncpy(ans_list[parking_id-1].answer[x].plate, plate, 32);
			ans_list[parking_id-1].answer[x].timestamp = timestamp;
			ans_list[parking_id-1].answer[x].micro_sec = micro_sec;
			ans_list[parking_id-1].state_changed = 1;
			break;
		}
	}

	if(x == 32)
	{//list full
		sprintf(log_msg,"[IPC] [Insert] [WARNING] park_id=%d, ipc list is full.\n", parking_id);
		Write_Log(0,log_msg);
		return 0;
	}

	ans_list[parking_id-1].list_inuse = -1;
	return 1;
}

int Insert_ch_list(int vs1_index, int ch_num, int tag)
{
	char		log_msg[MSG_SIZE];
	sprintf(log_msg,"[IPC] [Insert_ch] list[%d] = %d. \n", ch_num-1, vs1_index);
	Write_Log(0,log_msg);
	park_list[ch_num-1] = vs1_index;

	ans_list[ch_num-1].tag = tag;
	ans_list[ch_num-1].vs_index = vs1_index;
	pthread_mutex_init(&ans_list[ch_num-1].ans_list_mutex, 0);
	ans_list[ch_num-1].list_inuse = 0;
	return	ch_num;
}

int Clean_Answer_List(int parking_id, u32 timestamp, u32 micro_sec)
{
	int x = 0;
	char		log_msg[MSG_SIZE];
	sprintf(log_msg,"[IPC] [Clean_Answer] park_id=%d, ipc list clean, time=%d, usec=%d.\n", parking_id, timestamp, micro_sec);
	Write_Log(0,log_msg);

	//pthread_mutex_lock(&ans_list[parking_id-1].ans_list_mutex);
	if(ans_list[parking_id-1].list_inuse == 1)
	{
		//pthread_mutex_unlock(&ans_list[parking_id-1].ans_list_mutex);
		//return -1;
		usleep(50000);
	}
	ans_list[parking_id-1].list_inuse = 1;
	//pthread_mutex_unlock(&ans_list[parking_id-1].ans_list_mutex);


	for(x=0 ; x<MAX_CH ; x++)
	{
		memset(ans_list[parking_id-1].answer[x].plate, 0, 32);

		ans_list[parking_id-1].answer[x].timestamp = 0;
		ans_list[parking_id-1].answer[x].micro_sec = 0;

		ans_list[parking_id-1].start_timestamp = timestamp;
		ans_list[parking_id-1].start_micro_sec = micro_sec;
	}
	
	ans_list[parking_id-1].list_inuse = 0;
	return 0;
}

// -----------------------------------------------------------------------------
void	*Create_client(u32 ip, int port, char *ip_loc)
{
	gateway_c *hgc = (gateway_c *)malloc(sizeof(gateway_c));

	if(!hgc)
		return 0;

	memset(hgc, 0, sizeof(gateway_c));
	hgc->ip = ip;
	hgc->port = port;
	hgc->connnected = 0;

	strcpy(loc_ip, ip_loc);

	int x;
	for (x=0 ; x<MAX_CH; x++)
	{
		hgc->sck[x].park_id = -1;
		hgc->send_fg[x] = 0;
		hgc->PS_flag[x] = 0;
		hgc->send_max[x] = 0;
	}
	

	return hgc;
}

int		Gateway_Connect(void *hClient)
{
	char		log_msg[MSG_SIZE];
	int			ret;
	gateway_c *hgc = (gateway_c *)hClient;
	//printf();
	Write_Log(0,"[IPC] [GWC] Create hgc_thread (2) \n");

	hgc->first_conn = 1;
	//while(1)
	//{
		if((ret = create_new_thread(hgc_thread, hgc)) < 0) {
			//printf();
			sprintf(log_msg,"[IPC] [GWCE] cilent thread create failed. %s\n",strerror(errno));
			Write_Log(0,log_msg);
			free(hgc);
			sleep(1);
		}
		/*else
		{			
			break;
		}*/
	//}
	return 1;
}

int Gateway_Connect_Free(void *hClient)
{
	gateway_c *hgc = (gateway_c *)hClient;
	
// 	if(hgc->magic != CMD_MAGIC)
// 		return -1;
// 	
// 	hgc->magic = 0;
	return 0;
}

int	Send_List(void *hClient, int ch_id, int base_cout, int *sort_list, int sort_max, void *result_list, void* empty_ans, char *sip, int opt)
{
	gateway_c *hgc = (gateway_c *)hClient;
	char		log_msg[MSG_SIZE];

	result_info* ans_list = (result_info *)result_list;

	S_CMD	s_cm;

	char	cmd_header[64];
	char	msg[MSG_SIZE];

	struct tm tt;
	time_t	ts;
	char	str[32];
	int		re_val;
	int		ret = 0;
	int		num = 0;

	fd_set	  rds;
	struct timeval tv;
	int		  re_cout = 0;

	//pthread_mutex_lock( &client_mux );

	if(!hgc->connnected)
	{
		//printf();
		Write_Log(0,"[IPC] [GWC] hgc not connect .\n");
		re_val =  -1;
		goto Send_server_exit;
	}
	do 
	{
		FD_ZERO(&rds);
		FD_SET(hgc->sd, &rds);

		tv.tv_sec = 3;
		tv.tv_usec = 100000;
		while(1)
		{
			int x = 0;
			ret = select(hgc->sd+1, 0, &rds, 0, &tv);
			sprintf(log_msg,"[IPC] [GWC] [ch%04d] [SENDLIST] [1], select ret = %d %d. \n", ch_id,ret,x);
			Write_Log(0,log_msg);
			if (ret > 0)
			{
				sprintf(log_msg,"[IPC] [GWC] [ch%04d] [SENDLIST] [2], select ret = %d %d. \n", ch_id,ret,x);
				Write_Log(0,log_msg);
				break;
			}
			else if (ret <=0)
			{
				sprintf(log_msg,"[IPC] [GWC] [ch%04d] [SENDLIST] [3], select ret = %d %d. \n", ch_id,ret,x);
				Write_Log(0,log_msg);
				x++;
			}
			
			if(x == 3) 
			{
				sprintf(log_msg,"[IPC] [GWCE] [ch%04d] [SENDLIST] [4] send event - %s(%08x) select error: %s\n", ch_id, hgc->ip, inet_addr(hgc->ip), strerror(errno));
				Write_Log(0,log_msg);
				return -4;
			}		
		}

		if (opt == 1)
		{
			sprintf(log_msg,"[IPC] [GWC] [ch%04d], send empty list. \n", ch_id);
			Write_Log(0,log_msg);

			//sprintf(cmd_header, "|EventUpdate|%04d|1|tftp://192.168.1.101//lpr//2015050307/0014_3854925_ABC123.obi|", ch_id);
			sprintf(cmd_header, "|EventUpdate|%04d|0|", ch_id);
			memset( &(s_cm.h), 0, CMD_H_SZ );
			s_cm.h.dwMagic = CMD_MAGIC;
			s_cm.h.dwVersion = CMD_VERSION;
			s_cm.h.dwLen = strlen(cmd_header);

			memcpy(msg, &(s_cm.h), sizeof(CMD_H));
			sprintf(msg+sizeof(CMD_H), "%s", cmd_header);
			sprintf(log_msg,"[IPC] [GWC] [ch%04d], send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", ch_id, msg+sizeof(CMD_H), s_cm.h.dwLen+sizeof(CMD_H), s_cm.h.dwMagic,  s_cm.h.dwVersion, s_cm.h.dwLen);
			Write_Log(0,log_msg);

			if( (ret = send(hgc->sd, msg, s_cm.h.dwLen+sizeof(CMD_H), 0)) < s_cm.h.dwLen+sizeof(CMD_H) )
			{
				hgc->connnected = 0;
				sprintf(log_msg,"[IPC] [GWCE] [SEND] send event list send_msg(%d) fail ret=%d\n", strlen(msg), ret);
				Write_Log(0,log_msg);
				re_val =  -2;
				goto Send_server_exit;
			}
			else
			{
				sprintf(log_msg,"[IPC] [GWC] [SEND] send event list send_msg(%d) ret=%d\n", strlen(msg), ret);
				Write_Log(0,log_msg);
			}
		}
		else //if(opt == 0)
		{
			int ev_count = 0;
			int x;
			for (x=0 ; x<sort_max ; x++)
			{
				//if (opt == 2){sort_list[x] = x;}
				if (ans_list[sort_list[x]].cout > base_cout){ev_count++;}
				sprintf(log_msg,"[IPC] [GWC] [ch%04d], sort list count=%d, ans_list.cout = %d. \n",ch_id, ev_count, ans_list[sort_list[x]].cout);
				Write_Log(0,log_msg);
			}
			if (ev_count == 0)
				ev_count = ev_count + 1;
						//Count Lengh
			int ev_len = 0;
			sprintf(cmd_header, "|EventUpdate|%04d|%d|", ch_id, ev_count);

			ev_len = strlen(cmd_header);
			//sprintf(log_msg,"[IPC] [GWC] ch%d, send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d.\n", ch_id, msg+sizeof(CMD_H), ev_len, s_cm.h.dwMagic,  s_cm.h.dwVersion, s_cm.h.dwLen);
			//Write_Log(0,log_msg);
			for(x=0 ; x<ev_count ; x++)
			{
				if( (sort_max>1) && (ev_count==1) )
				{
					if( (strcmp(ans_list[sort_list[x]].plate, "##-####")) == 0)
						x++;
				}

				ts = ans_list[sort_list[x]].timestamp;
				localtime_r(&ts, &tt);

				sprintf(str, "%04d%02d%02d%02d/%04d_%02d%02d%03d",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, ch_id, tt.tm_min, tt.tm_sec, ans_list[sort_list[x]].micro_sec/1000);

				sprintf(cmd_header, "tftp://%s//lpr//%s_%s.obi|", sip, str, ans_list[sort_list[x]].plate);

				/*sprintf(str, "%04d%02d%02d%02d%02d%02d%03d",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec, ans_list[sort_list[x]].micro_sec/1000);

				sprintf(cmd_header, "tftp://%s//lpr//%04d_%s_%s.obi|", sip, ch_id, str, ans_list[sort_list[x]].plate);*/
				sprintf(log_msg,"[IPC] [GWC] [ch%04d], cmd_header=%s(%d). \n", ch_id, cmd_header, strlen(cmd_header));
				Write_Log(1,log_msg);

				sprintf(msg+ev_len, "%s", cmd_header);
				ev_len = ev_len + strlen(cmd_header);

				sprintf(log_msg,"[IPC] [GWC] [ch%04d], send_msg=%s(%d) <Cout>. \n", ch_id, msg+sizeof(CMD_H), ev_len);
				Write_Log(1,log_msg);
			}
			//Ready msg & header
			memset( &(s_cm.h), 0, sizeof(CMD_H) );
			s_cm.h.dwMagic = CMD_MAGIC;
			s_cm.h.dwVersion = CMD_VERSION;
			s_cm.h.dwLen = ev_len;

			memset( msg, 0, MSG_SIZE );
			memcpy(msg, &(s_cm.h), sizeof(CMD_H));
			sprintf(cmd_header, "|EventUpdate|%04d|%d|", ch_id, ev_count);
			//if(opt==0){sprintf(cmd_header, "|EventUpdate|%04d|%d|", ch_id, ev_count);}
			//else if(opt==2){sprintf(cmd_header, "|SendMaxAns|%04d|%d|", ch_id, ev_count);}
			sprintf(msg+sizeof(CMD_H), "%s", cmd_header);
			ev_len = strlen(cmd_header)+sizeof(CMD_H);
			//sprintf(log_msg,"[IPC] [GWC]  [ch%04d], ev_len = %d %d %d %d. \n", ch_id, ev_len,sizeof(CMD_H),msg[15],msg[16]);
			//Write_Log(1,log_msg);
			for(x=0 ; x<ev_count ; x++)
			{
				if( (sort_max>1) && (ev_count==1) )
				{
					if( (strcmp(ans_list[sort_list[x]].plate, "##-####")) == 0)
						x++;
				}

				ts = ans_list[sort_list[x]].timestamp;
				localtime_r(&ts, &tt);

				sprintf(str, "%04d%02d%02d%02d/%04d_%02d%02d%03d",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, ch_id, tt.tm_min, tt.tm_sec, ans_list[sort_list[x]].micro_sec/1000);

				sprintf(cmd_header, "tftp://%s//lpr//%s_%s.obi|", sip, str, ans_list[sort_list[x]].plate);

				/*sprintf(str, "%04d%02d%02d%02d%02d%02d%03d",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec, ans_list[sort_list[x]].micro_sec/1000);

				sprintf(cmd_header, "tftp://%s//lpr//%04d_%s_%s.obi|", sip, ch_id, str, ans_list[sort_list[x]].plate);*/
				sprintf(log_msg,"[IPC] [GWC] [ch%04d], cmd_header=%s(%d). \n", ch_id, cmd_header, strlen(cmd_header));
				Write_Log(1,log_msg);

				sprintf(msg+ev_len, "%s", cmd_header);

				ev_len = ev_len + strlen(cmd_header);

				sprintf(log_msg,"[IPC] [GWC] [ch%04d], send_msg=%s(%d) <Ready>. \n", ch_id, msg+sizeof(CMD_H), ev_len);
				Write_Log(1,log_msg);
			}
			sprintf(log_msg,"[IPC] [GWC] [BEF] [ch%04d], send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", ch_id, msg+sizeof(CMD_H), s_cm.h.dwLen+sizeof(CMD_H), s_cm.h.dwMagic,  s_cm.h.dwVersion, s_cm.h.dwLen);
			Write_Log(0,log_msg);
			if( (ret = send(hgc->sd, msg, s_cm.h.dwLen+sizeof(CMD_H), 0)) < s_cm.h.dwLen+sizeof(CMD_H))
			{
				sprintf(log_msg,"[IPC] [GWCE] [SEND] send event list send_msg - %s(%d) fail ret=%d\n", msg+sizeof(CMD_H), s_cm.h.dwLen+sizeof(CMD_H), ret);
				Write_Log(0,log_msg);
				hgc->connnected = 0;
				re_val =  -2;
				goto Send_server_exit;
			}

			sprintf(log_msg,"[IPC] [GWC] [AFT] [ch%04d], send_msg=%s(%d), s_cm.Magic=%X, dwVersion=%X dwLen=%d. \n", ch_id, msg+sizeof(CMD_H), s_cm.h.dwLen+sizeof(CMD_H), s_cm.h.dwMagic,  s_cm.h.dwVersion, s_cm.h.dwLen);
			Write_Log(1,log_msg);	
		}
		//recv feedback msg
		do{

			FD_ZERO(&rds);
			FD_SET(hgc->sd, &rds);

			tv.tv_sec = 1;
			tv.tv_usec = 100000;

			ret = select(hgc->sd+1, &rds, 0, 0, &tv);

			if(ret < 0) 
			{
				if(errno != EINTR) {
					sprintf(log_msg,"[IPC] [GWCE] [ch%04d] send event - %s(%08x) select error: %s\n", ch_id, hgc->ip, inet_addr(hgc->ip), strerror(errno));
					Write_Log(0,log_msg);
				}
				return -4;
			}
			else if(ret == 0 || !FD_ISSET(hgc->sd, &rds))
			{
				sprintf(log_msg,"[IPC] [GWCE] [ch%04d] send event - %s(%08x) select return=0 : %s\n", ch_id, hgc->ip, inet_addr(hgc->ip), strerror(errno));
				Write_Log(0,log_msg);
				re_cout++;
			}
			else 
			{
				sprintf(log_msg,"[IPC] [GWC] [ch%04d] send event - %s(%08x) select ret = %d\n", ch_id, hgc->ip, inet_addr(hgc->ip),ret);
				Write_Log(0,log_msg);
				break;
			}
			if(re_cout >= 30)
			{
				num++;
				break;
			}
		}while(ret == 0);
		sprintf(log_msg,"[IPC] [GWC] [ch%04d] send list failed - try again %s(%08x) select num = %d\n", ch_id, hgc->ip, inet_addr(hgc->ip),num);
		Write_Log(0,log_msg);
		if(num > 2)
			return -5;
	} while (ret == 0);		

	CMD_H *fh = (CMD_H *)msg;
	ret = recv(hgc->sd, msg, MSG_SIZE, 0);

	if (ret < 0)
	{
		hgc->connnected = 0;
		sprintf(log_msg,"[IPC] [GWCE] [RECV] recv msg from server faill ret=%d\n", ret);
		Write_Log(0,log_msg);
		re_val =  -3;
		goto Send_server_exit;
	}
	else if (ret == 0)
	{
		sprintf(log_msg,"[IPC] [GWCE] ret = 0 .\n");
		Write_Log(0,log_msg);
	}
	else
	{
		sprintf(log_msg,"[IPC] [GWC] [RECV] recv msg-%s(%d) from server .\n", msg+sizeof(CMD_H), fh->dwLen);
		Write_Log(0,log_msg);
	}

	re_val = 1;
	
Send_server_exit:
	//pthread_mutex_unlock( &client_mux );
	
	return re_val;
}

void	Set_sck_param(void* gc, int park_id, int base_cout, int top_cout, result_info* ans, result_info *empty_ans, pthread_mutex_t *mux)
{
	gateway_c *hgc = (gateway_c*)gc;

	hgc->sck[park_id-1].park_id = park_id;
	hgc->sck[park_id-1].vote_list = ans;
	hgc->sck[park_id-1].base_cout = base_cout;
	hgc->sck[park_id-1].top_cout = top_cout;
	hgc->list_mutex[park_id-1] = mux;
	hgc->send_fg[park_id-1] = 1;

	hgc->sck->empty_ans = empty_ans;

	//printf("[GWC] [PARAM]")
}

int		Set_flag(void* gc, int park_id)
{
	gateway_c *hgc = (gateway_c*)gc;
	int a = 0;	
	//while (1)
	//{
		if (hgc->send_fg[park_id-1] ==0)
		{
			hgc->send_fg[park_id-1] = 1;
			//return 0;
		}
		//a++;
		//if (a==5 && hgc->send_fg[park_id-1] == 1){hgc->PS_flag[park_id-1] = 1 ; break;}
		//usleep(6000000);
	//}


	return 0;
}

int		Set_max(void* gc, int park_id,int maxtype)
{
	char		log_msg[MSG_SIZE];
	sprintf(log_msg,"[IPC] [Set_max] IN .\n");
	Write_Log(0,log_msg);
	if (gc != NULL)
	{
		gateway_c *hgc = (gateway_c*)gc;

		memset(temp_ans[park_id-1].ans, 0 ,sizeof(result_info)*MAX_CH);
		memcpy(temp_ans[park_id-1].ans, hgc->sck[park_id-1].vote_list, sizeof(result_info)*MAX_CH);
		hgc->send_max[park_id-1] = maxtype;
		if (maxtype==1)
		{
			temp_ans[park_id-1].send_flag = 1;
		}
		sprintf(log_msg,"[IPC] [Set_max] [ch%04d] maxtype is %d ,Return List.\n",park_id ,maxtype);
		Write_Log(0,log_msg);
	}

	return 0;
}

int		Set_Vsfail(int *state , int total_num)
{
	char		log_msg[MSG_SIZE];
	int			cout = 0;
	
	vsf = state;
	sprintf(log_msg,"[IPC] [Set_Vsfail] IN state %d %d.\n",total_num, vsf[0]);
	Write_Log(0,log_msg);	

	if (state == NULL)
	{
		sprintf(log_msg,"[IPC] [Set_Vsfail] state is NULL\n");
		Write_Log(0,log_msg);
		return -1;
	}
	
	//int i;
	//for (i=0;i<vsf[0];i++)
	//{
	//	sprintf(log_msg,"[IPC] [Set_Vsfail] %d\n", vsf[i]);
	//	Write_Log(0,log_msg);
	//}
	Total_Num = vsf[0];
	Vs_flag = 1;
	return 0;
}
