#include "ob_platform.h"
#include "dbg_msg.h"

#ifndef MAX_CH
#define MAX_CH 32
#endif

#define __USE_LARGEFILE64

typedef struct
{	
	int		log_inuse;
	int		fLog;
}msg_ctrl;

#define TIME_SLOT   3600

// record time
static u32 rec_min_ts = 0xffffffff;
static u32 rec_min_ts2 = 0xffffffff;
static u32 rec_max_ts = 0;

static int min_space = 1000;
static char log_path[64] = "";
static u32 init_time;

msg_ctrl* f;

int running;
// ------------------------------------------------------------------

static void mk_prefix(char *str, time_t ts)
{
	struct tm tt;
	localtime_r(&ts, &tt);
	sprintf(str, "%04d%02d%02d%02d00",
		tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour);
}

static void mk_filename(char *str, u8 id, time_t ts)
{
	// 1 hour per file
	struct tm tt;
	//gmtime_r(&ts, &tt);
	localtime_r(&ts, &tt);
	sprintf(str, "%04d%02d%02d%02d00-%03u",
		tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, id);
}

static u32 filename2time(const char *name)
{
	if(strlen(name)<12)
		return 0;
	struct tm tt;
	memset(&tt, 0, sizeof(tt));
	sscanf(name, "%04d%02d%02d%02d", &tt.tm_year, &tt.tm_mon, &tt.tm_mday, &tt.tm_hour);
	if(tt.tm_year < 1900)
		return 0;
	tt.tm_year -= 1900;
	if(tt.tm_mon < 1 || tt.tm_mon > 12)
		return 0;
	tt.tm_mon -= 1;
	if(tt.tm_mday < 1 || tt.tm_mday > 31)
		return 0;
	if(tt.tm_hour > 23)
		return 0;
	return (u32)mktime(&tt);
}

static int free_space(const char *path)
{
	struct statfs st;
	if(statfs(path, &st) == 0)
		return (int)(st.f_bfree / ((1024*1024)/st.f_bsize));
	return -1;
}

static int valid_path(const char *path)
{
	struct stat st;
	if(stat(path, &st) < 0 || !S_ISDIR(st.st_mode))
		return 0;
	return 1;
}

static int find_time(const char *path, u32 *min, u32 *min2, u32 *max)
{
	// find oldest file time
	DIR *dir = opendir(path);
	if(dir == 0)
		return -1;

	u32 mnt = 0xffffffff;
	u32 mnt2 = 0xffffffff;
	u32 mxt = 0;
	struct dirent *dent;

	while((dent = readdir(dir)) != 0) {
		u32 ts = filename2time(dent->d_name);
		if(ts == 0)
			continue;
		if(ts < mnt) {
			mnt2 = mnt;
			mnt = ts;
		}
		if(ts > mxt)
			mxt = ts;
	}

	closedir(dir);

	if(min)
		*min = mnt;
	if(min2)
		*min2 = mnt2;
	if(max)
		*max = mxt;
	return 0;
}

static int recycle(const char *path)
{
	// find oldest file time
	u32 min;
	find_time(path, &min, 0, 0);

	if(min == 0xffffffff)
		return 0;

	DIR *dir;
	struct dirent *dent;
	char prefix[20];
	mk_prefix(prefix, min);
	int plen = strlen(prefix);

	ob_debug("recycle: min = %u, %s\n", min, prefix);

	// delete oldest files
	dir = opendir(path);
	if(dir == 0)
		return -1;

	while((dent = readdir(dir)) != 0) {
		if(strncmp(prefix, dent->d_name, plen) == 0) {
			char name[256];
			sprintf(name, "%s/%s", path, dent->d_name);
#if 1
			if(unlink(name) < 0) {
				ob_error("delete %s failed: %s\n", name, strerror(errno));
				// try to delete file by system()
				char cmd[256];
				sprintf(cmd, "rm -rf %s", name);
				system(cmd);
			}
			else {
				ob_debug("delete %s\n", name);
			}
#else
			ob_debug("recycle: %s\n", name);
#endif
		}
	}

	closedir(dir);

	return 0;
}

// -----------------------------------------------------------------------
static int log_f_open(void *priv, int ch_id, u32 time)
{
	msg_ctrl *file = (msg_ctrl *)priv;
	//assert(filename);

	char	file_path[64], file_time[32];
	struct stat sts;
	time_t tt;
	tt = time;

	mk_filename(file_time, ch_id, tt);
	sprintf(file_path, "%s/%s.log", log_path, file_time);

	if (stat(file_path, &sts) == -1 && errno == ENOENT)
	{//file not exist
		printf ("The file %s doesn't exist...\n", file_path);
		file->fLog = open(file_path, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE | O_SYNC);

		if(!file->fLog)
		{
			printf("[DBG] [ERROR] open log failed: %s\n", file_path, strerror(errno));
			return -1;
		}
	}
	else
	{
		file->fLog = open(file_path, O_WRONLY | O_APPEND | O_TRUNC | O_LARGEFILE | O_SYNC);
		if(!file->fLog)
		{
			printf("[DBG] [ERROR] open log failed: %s\n", file_path, strerror(errno));
			return -2;
		}
	}

	return 0;
}
// ------------------------------------------------------------------

static void *check_free_thread(void *arg)
{
	ob_info("thread start .....PID: %u\n", (u32)gettid());

	int x, ret;
	struct timeval tt;
	u32 now , hour;
	

	while(running) {

		// check fs stat for recycle
		if(!valid_path(log_path)) {
			ob_debug("%s not exist\n", log_path);
			continue;
		}

		//check time for create new file
		gettimeofday(&tt, 0);
		now = (u32) tt.tv_sec;

		hour = now / TIME_SLOT;
		if( hour > init_time)
		{
			for(x=0 ; x<MAX_CH ; x++)
			{
				ret = log_f_open(f, x, now);
			}
			init_time = hour;
		}

		struct statfs st;
		u32 r_free = 0;
		u32 r_size = 0;
		u32 r_used = 0;
		if(statfs(log_path, &st) == 0) {
			r_free = (u32)(st.f_bfree / ((1024*1024)/st.f_bsize));
			r_size = (u32)(st.f_blocks / ((1024*1024)/st.f_bsize));
			r_used = (u32)((st.f_blocks - st.f_bavail) / ((1024*1024)/st.f_bsize));
			ob_debug("Size=%uMB, Used=%uMB, Free=%uMB, Low Level=%uMB\n", r_size, r_used, r_free, min_space);
		}
		else {
			ob_error("get fsstat failed: %s\n", strerror(errno));
		}
			
		find_time(log_path, &rec_min_ts, &rec_min_ts2, &rec_max_ts);
		if(r_free < min_space) {
		recycle(log_path);
		}
		
		sleep(60);
	}

	return 0;
}

// -------------------------------------------------------------------------------
/*void log_file_time(u32 sec, u32 *start, u32 *duration)
{
	time_t ts = (time_t)sec;
	struct tm tt;
	//gmtime_r(&ts, &tt);
	localtime_r(&ts, &tt);
	tt.tm_min = tt.tm_sec = 0;
	if(start)
		*start = (u32)mktime(&tt);
	if(duration)
		*duration = TIME_SLOT;
}
*/
// -------------------------------------------------------------------------------
void *Open_Msg(int total_num, char *log_path)
{
	f = (msg_ctrl *)malloc(sizeof(msg_ctrl)*MAX_CH);

	if(!f)
		return 0;

	int x, ret;
	struct stat sts;


	struct timeval tt;
	gettimeofday(&tt, 0);
	u32 now = (u32) tt.tv_sec;

	for(x=0 ; x<total_num ; x++)
	{
		if(!valid_path(log_path))
			f[x].log_inuse = 0;	//return -1;

		ret = pow(2, x);
		if( total_num & ret > 0)
			f[x].log_inuse = 1;

		ret = log_f_open(f, x, now);
		/*
		if(!valid_path(r->path))
		return -1;
		mk_filename(r->prefix, r->rid, fh->sec);
		sprintf(r->filename, "%s/%s.ovs", r->path, r->prefix);
		u32 start, duration;
		rec_file_time(fh->sec, &start, &duration);
		r->vf_handle = obf_open(r->filename, 0, start, duration);
		if(r->vf_handle == 0) {
		ob_error("[%d] open %s error\n", r->rid, r->filename);
		return -1;
		}
		*/
	}
	init_time = now / TIME_SLOT;

	
	create_thread(check_free_thread, 0);
	return f;
}

int Write_Msg(int ch_id, char *msg, int size)
{
	int wsize;
	if((wsize = write(f[ch_id-1].fLog, msg, size)) < 0) {
		dbg("write %u bytes to %d from %p error:%s\n",
			size, f[ch_id-1].fLog, msg, strerror(errno));
		return -1;
	}
	return 0;
}

int  Close_Msg(void *file)
{
	int x;

	for (x=0 ; x<MAX_CH ; x++)
	{
		close(f[x].fLog);
	}
	
	free( f );
}

