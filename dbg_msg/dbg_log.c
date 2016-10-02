//#include "as_protocol.h"
#include "ob_platform.h"
#include "dbg_log.h"
#include "lpr/OBWinDef.h"
#include "lpr/OBErrMsg.h"
#include "lpr/OBLogFil.h"
#include "lpr/OBImgPrc.h"
#include "lpr/CVWnLPR.h"
#include "lpr/CP_LprFrm.h"
#include <pthread.h>

//#ifndef MAX_CH
//#define MAX_CH 32
//#endif
//
//#define __USE_LARGEFILE64
//
//typedef struct
//{	
//	int		log_inuse;
//	int		fLog;
//}msg_ctrl;

#define TIME_SLOT   3600
#define INI			"3531.ini"

// record time
//static u32 rec_min_ts = 0xffffffff;
//static u32 rec_min_ts2 = 0xffffffff;
//static u32 rec_max_ts = 0;
//
//static int min_space = 1000;
//static char log_path[64] = "";
//static u32 init_time;

typedef struct
{
	int ch_idx;
	int secx;
}del_file;


//msg_ctrl* f;
int		hour = 24;
int		yesec = 0;
int		wsec = 0;
int		change_log;
del_file dfile;
//int running;
//=======================================
int check24hour(void *arg)
{
	//printf("check24hour in\n");
	del_file *dfile = (del_file*)arg;
	struct	tm *timeyes;
	struct	tm *timeweek;
	char	time_buffer_yes[64];
	char	time_buffer_week[64];
	char	file_name_yes[128];
	char	dpath[64];
	char	check_path[64];
	int		count = 0;
	DIR *dir;
	struct dirent *dent;

	//GetPrivateProfileString( "System", "GW_Server", "0", gw_ip, 32, INI );
	//sprintf(time_path,"./ntpclient -s -t -i 10 -h %s",gw_ip);
	//system(time_path);

	yesec = dfile->secx-86400;
	timeyes = localtime(&yesec);
	strftime (time_buffer_yes, 80, "%Y%m%d%H", timeyes);

	sprintf(file_name_yes,"%s0000.log",time_buffer_yes);	
		do 
		{
			dir = opendir("/dbg/log/");
			count++;
			printf("[FILE] Open dir fail = %s(%d)!\n",strerror(errno),count);
		} while (dir == 0);
		while((dent = readdir(dir)) != 0) 
		{
			//printf("find file %s\n",dent->d_name);
			if(strncmp(file_name_yes, dent->d_name, strlen(file_name_yes)) == 0) 
			{
				char cmd[128];
				sprintf(cmd, "/dbg/log/%s",file_name_yes);
				//remove(dent->d_name)
				if(remove(cmd)==0)
				{
					printf("[FILE] Remove Log Success!\n");
				}
			}
		}
		char	path[256];
		sprintf(path,"rm -rf /dbg/lpr/%s",time_buffer_yes);
		printf("[FILE] %s!\n",path);
		system(path);
		sprintf(check_path,"echo [FILE] %s! > ./dlpr.log",path);
		system(check_path);
		sleep(5);
		closedir(dir);		


		//wsec = dfile->secx-(86400*15);
		//timeweek = localtime(&wsec);
		//strftime (time_buffer_week, 80, "%Y%m%d%H", timeweek);

		//sprintf(dpath,"rm -rf /mnt/data/%s*",time_buffer_week);
		//system(dpath);
		//sprintf(check_path,"echo [FILE] %s! > ./boot.log",dpath);
		//system(check_path);

	pthread_exit(0);
}

int  Write_Log(int ch_id, char *msg)
{
	struct timeval tt;
	struct tm tv;
	gettimeofday(&tt, NULL);
	u32 now = (u32) tt.tv_sec;
	//struct	tm *timeinfo;
	char	time_buffer[128];
	char	file_name[128];
	char	load_log[64];
	char	load_time[32];
	char	ans_msg[1024];
	char	path[128];
	int		ret,mkd,reboottime;
	FILE	*fp = 0;
	localtime_r(&now,&tv);
	sprintf(ans_msg,"[%02d:%02d] %s ",tv.tm_min, tv.tm_sec,msg);

	GetPrivateProfileString( "System", "Control_Log", "2", load_log, 32, INI );
 	ret = atoi(load_log);

	sprintf(time_buffer,"%04d%02d%02d%02d", tv.tm_year+1900, tv.tm_mon+1, tv.tm_mday, tv.tm_hour);
	sprintf(file_name,"/dbg/log/%s0000.log",time_buffer);
	pthread_t thread_id;

	if (ch_id == 3)
	{
		if (change_log == 1)
		{
			printf("[FILE] Change Log %d o'clock!\n",hour);
			dfile.ch_idx = ch_id;
			dfile.secx = now;

			if(pthread_create(&thread_id, NULL, check24hour, &dfile) != 0) 
			{
				return -1;
			}
			pthread_detach(thread_id);


			if((ret=chmod(file_name,S_ISGID|S_IWUSR|S_IXUSR|S_IRGRP|S_IROTH))<0)
			{
				printf("[FILE] chmod Log fail\n");
			}
			sprintf(path,"/dbg/lpr/%s",time_buffer);

			if((mkd=mkdir(path,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
			{
				printf("[DBG] mkdir dbg/lpr/ fail\n");
			}
			change_log = 0;
		}
	}	
	else if(ch_id <= ret)
	{	
		if(tv.tm_hour != hour && (tv.tm_year+1900)!=1970)
		{
			change_log = 1;
			if (hour!=24)
			{
				GetPrivateProfileString( "System", "Reboot_Time", "0", load_time, 32, INI );
				reboottime = atoi(load_time);
				if (reboottime == tv.tm_hour)
				{
					printf("[FILE] 0'clock Begin reboot Reboot_Time is %d Now is %d\n",reboottime,hour);
					sleep(1);
					system("reboot");
				}

			}	

			hour=tv.tm_hour;

			//printf("[FILE] %s\n",file_name);
			fp = fopen(file_name, "ab+");

			if(fp) {
					//printf("[TIME] %s\n",time_buffer);
					fwrite(ans_msg, 1, strlen(ans_msg), fp);
				   }
					fflush(fp);
		}
		else {
			fp = fopen(file_name, "ab+");
			if(fp) {
					//printf("[TIME] %s\n",time_buffer);
					fwrite(ans_msg, 1, strlen(ans_msg), fp);
				   }
					fflush(fp);
			}
		fclose(fp);
	}
		return 0 ;
}