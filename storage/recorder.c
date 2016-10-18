/*!
 *  @file       recorder.c
 *  @version    1.0
 *  @date       04/01/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       Recorder <br>
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "ob_platform.h"
#include "vsc/vsc.h"
#include "ob_recorder.h"
#include "ob_file.h"
#include "ob_event.h"
#include "codec/codec.h"
#include "dbg_msg/dbg_log.h"


// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "REC"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------
typedef unsigned char		BYTE;
#define REC_MAGIC   0x7245634d      // 'rEcM'

#define TIME_SLOT   3600
#define EST_SIZE    (TIME_SLOT / 8)
// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------
static int running = 1;
static char rec_path[64] = "";
static int min_space = EST_SIZE;

static pthread_mutex_t mutex_ev_v = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_ev_p = PTHREAD_MUTEX_INITIALIZER;
static void *ef_plate = 0;
static void *ef_video = 0;
static char efp_filename[FN_SZ];
static char efv_filename[FN_SZ];

typedef struct
{
	int	m_count;
}rec_count;

//#define SAVE_PLATE
static int ch_nr = 0;
rec_count	rcount[32];
int		grab_flag = 0;
int		count = 1;
//u8		temp_data[400000];
static char *plate_str[32] = {0};
char			time_path[256];
char			my_ip[32];
u32				my_ipaddress;
unsigned char *g;

//snapshot



// record time
static u32		rec_min_ts = 0xffffffff;
static u32		rec_min_ts2 = 0xffffffff;
static u32		rec_max_ts = 0;
static int		rec_fps[32];
static int      park_list[32];

//static char				log_msg[256];

// ----------------------------------------------------------------------------
// Motion Detect Functions
// ----------------------------------------------------------------------------

//int Resize(u8 data,int rwidth, int rheight)
//{
//	void *jbuf;
//	int jsize;
//	int twidth = 1920;
//	int theight = 1080;
//	struct tm tt;
//	char time[128];
//
//	localtime_r(&ts, &tt);
//	sprintf(time, "/motion/%04d%02d%02d%02d%02d%02d000",
//		tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour,tt.tm_min,tt.tm_sec);
//
//	sprintf(log_msg,"[RC] [Motion] heigh %d width = %d jsize = %d \n", rheight , rwidth,jsize);
//	Write_Log(0,log_msg);	
//	jsize = twidth*theight;
//	jbuf = malloc(jsize);
//
//	int psize = h264tojpeg(data, jsize, jbuf, jsize, 0, 0, twidth, theight);
//
//
//	FILE *fp = fopen(time,"wb");
//	if (fp)
//	{
//		fwrite(jbuf,jsize,1,fp);
//		fclose(fp);
//	}
//
//}

int Insert_rec_list(int vs1_index, int ch_num)
{
	char		log_msg[256];
	sprintf(log_msg,"[RC] [Insert_rec_ch] list[%d] = %d. \n", ch_num-1, vs1_index);
	Write_Log(0,log_msg);
	park_list[ch_num-1] = vs1_index;
	return park_list;
}

// ----------------------------------------------------------------------------
// get_ip Functions
// ----------------------------------------------------------------------------
static int get_ip(const char *interface, u32 *ifaddr)
{
	struct ifreq ifr;
	struct sockaddr_in *device_data;
	strcpy(ifr.ifr_name, interface);
	*ifaddr = 0;

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0) {
		printf("[SCK] Open INET socket error\n");
		return -1;
	}

	if(ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
		printf("[SCK] Could not get IF address for %s\n", ifr.ifr_name);
		close(sd);
		return -1;
	}
	device_data=(struct sockaddr_in*)&(ifr.ifr_addr);
	sprintf(my_ip,"%s",inet_ntoa(device_data->sin_addr));
	printf("Current IP = %s\n",my_ip);
//	*ifaddr = ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
//printf("[SCK] %s IP is 0x%x , %d\n", interface, *ifaddr, *ifaddr);	
	close(sd);
	return 0;
}
// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
static void mk_jpegname(char *str, u8 id, time_t ts)
{
	// 1 hour per file
	struct tm tt;
	//gmtime_r(&ts, &tt);
	localtime_r(&ts, &tt);
	sprintf(str, "%03u-%04d%02d%02d%02d%02d%02d", id,
		tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
}
static void mk_prefix(char *str, time_t ts)
{
    struct tm tt;
    localtime_r(&ts, &tt);
    sprintf(str, "%04d%02d%02d%02d%02d%03d",
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
	char				log_msg[256];
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

    //ob_debug("recycle: min = %u, %s\n", min, prefix);
	sprintf(log_msg,"[RC] recycle: min = %u, %s\n", min, prefix);
	Write_Log(0,log_msg);

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
                //ob_error("delete %s failed: %s\n", name, strerror(errno));
				sprintf(log_msg,"[RC] delete %s failed: %s\n", name, strerror(errno));
				Write_Log(0,log_msg);
                // try to delete file by system()
                char cmd[256];
                sprintf(cmd, "rm -rf %s", name);
                system(cmd);
            }
            else {
                //ob_debug("delete %s\n", name);
				sprintf(log_msg,"[RC] delete %s\n", name);
				Write_Log(0,log_msg);
            }
#else
            //ob_debug("recycle: %s\n", name);
			sprintf(log_msg,"[RC] recycle: %s\n", name);
			Write_Log(0,log_msg);
#endif
        }
	}

	closedir(dir);

    return 0;
}

static int rec_release(rec_t *r)
{
    if(r->vs_handle) {
        vsc_free(r->vs_handle);
        r->vs_handle = 0;
    }
    if(r->vf_handle) {
        obf_close(r->vf_handle);
        r->vf_handle = 0;
    }
    r->magic = 0;
    free(r);
    return 0;
}

static int rec_callback(void *priv, void *header, u8 *data)
{
	char				log_msg[256];
	BYTE				buf_test[500000];
	BYTE				jbuf[500000];
	int					psize;
	int					jsize;
	int					go = 0;
	int					motion = 0;
	int					rid;
	//LPRECT				lpSrc;

	rec_t *r = (rec_t *)priv;
	as_frame_t *fh = (as_frame_t *)header;	
	struct timeval now;
	gettimeofday(&now, 0);
	rid=park_list[r->rid];
	sprintf(log_msg,"[RC] r->rid = %d rid = %d r->ch_index = %d\n",r->rid,rid,r->ip,r->ch_index);
	Write_Log(0,log_msg);
	if(!fh || fh->size == 0) {
		if(r->vsig >= 0) { // connected -> disconnected
			rec_event_video(r->rid, 1);
			r->vsig = -1;
		}
		return 0;
	}
	//sprintf(log_msg,"[RC] [TEST] [recch%03d] rec_fps = %d data[0]== %d && data[1]== %d && data[2]== %d && data[3]== %d && data[4]== %d\n",r->rid, rec_fps[r->rid],data[0],data[1],data[2],data[3],data[4]);
	//Write_Log(0,log_msg);
	r->heigh = fh->height;
	r->width = fh->width;
	jsize = (r->heigh)*(r->width);
	memset(jbuf,0,500000);
	memset(buf_test,0,500000);
	if (data[0]== 00 && data[1]== 00 && data[2]==00 && data[3]==01 && data[4]==103){go = 1;} 
	else if(data[0]== 00 && data[1]== 00 && data[2]==00 && data[3]==01 && data[4]==97){go = 1;}
	else{go = 0;}
	
	if (rec_fps[r->rid] == 0 && go == 1)
	{
		//Motion 
		if (r->motion_state==1)
		{
			if (rcount[rid].m_count<5)
			{
				memcpy(buf_test,data,jsize);
				psize = h264tojpeg(buf_test, jsize , jbuf, jsize, 0, 0, r->width, r->heigh,0,0);
				sprintf(log_msg,"[RC] %d real size = %d,Out size = %d,x = %d,y = %d,width = %d,height = %d\n",rcount[rid].m_count, jsize, psize, 0, 0,r->width, r->heigh);
				Write_Log(0,log_msg);
				if (psize>0 )
				{
					FILE *fp1;
					char mpath[256];
					mk_jpegname(time_path, r->ip-171256576, now.tv_sec);
					sprintf(mpath,"/dbg/vs/%s-%s-%d.jpg",my_ip,time_path,rcount[rid].m_count);
					//printf("%s\n",mpath);
					if(fp1 = fopen(mpath,"wb"))
					{
						fwrite(jbuf,psize,1,fp1);
						fclose(fp1);
					}
					rcount[rid].m_count++;
				}
			}
			else
			{
				r->motion_state=0;
				rcount[rid].m_count = 0;
			}
		}
		//
		sprintf(log_msg,"[RC] [IN] heigh %d width = %d jsize = %d \n", r->heigh , r->width,jsize);
		Write_Log(0,log_msg);	
		//sprintf(log_msg,"[Buffer Bytes] %u %u %u %u %u\n",data[0],data[1],data[2],data[3],data[4]);
		//Write_Log(0,log_msg);
		if (r->buf_state ==0)
		{
			r->buf_state = 1;
			//g = r->buf_snap;
			memcpy(r->buf_snap,data,jsize);
			r->buf_state = 0;
		}
		sprintf(log_msg,"[RC] ch_id = %d rec_fps = %d psize = %d\n",r->rid, rec_fps[r->rid] , psize);
		Write_Log(0,log_msg);
		rec_fps[r->rid]++;
		if(!r || r->magic != REC_MAGIC)
			return -1;
		if(!running) {
			rec_release(r);
			return 0;
		}

		if(r->vsig < 1) { // disconnected -> connected
			rec_event_video(r->rid, 0);
			r->vsig = 1;
		}

		// time adjustment
		i64 ts = ((u64)now.tv_sec) * 1000000 + now.tv_usec;
		i64 vts = ((u64)fh->sec) * 1000000 + fh->usec;
		i64 vdelta = vts - r->last_vts;
		i64 nts = r->last_ts + vdelta;
		i64 delta = nts - ts;
		if(delta < -1000000 || delta > 1000000) {
			fh->sec = (u32)now.tv_sec;
			fh->usec = (u32)now.tv_usec;
		}
		else {
			fh->sec = (u32)(nts / 1000000);
			fh->usec = (u32)(nts % 1000000);
		}

		//ob_debug("[%d] %u.%06u @ %u.%06u (%02x)\n", r->rid, fh->sec, fh->usec, (u32)now.tv_sec, (u32)now.tv_usec, fh->flags);

		r->last_ts = ts;
		r->last_vts = vts;

		do {
			if(r->vf_handle == 0) {
				if(!valid_path(r->path))
					break;
				mk_filename(r->prefix, r->rid, fh->sec);
				sprintf(r->filename, "%s/%s.ovs", r->path, r->prefix);
				u32 start, duration;
				rec_file_time(fh->sec, &start, &duration);
				r->vf_handle = obf_open(r->filename, 0, start, duration);
				if(r->vf_handle == 0) {
					//ob_error("[%d] open %s error\n", r->rid, r->filename);
					sprintf(log_msg,"[RC] [%d] open %s error\n", r->rid, r->filename);
					Write_Log(0,log_msg);
					break;
				}
				//ob_debug("%s : %u / %u\n", r->filename, start, duration);
				sprintf(log_msg,"[RC] %s : %u / %u\n", r->filename, start, duration);
				Write_Log(0,log_msg);
			}


			int ret = obf_write(r->vf_handle, fh, data);
			if(ret > 0)
				break;
			else if(ret == 0) {
				obf_close(r->vf_handle);
				r->vf_handle = 0;
				continue;
			}
			else
				break;
		} while(1);
	}
	else if(rec_fps[r->rid] == r->fps)
	{
		rec_fps[r->rid] = 0;
	}
	else	
		rec_fps[r->rid]++;
    return 0;
}

static void *rec_thread(void *arg)
{
	char				log_msg[256];
    //ob_info("thread start .....PID: %u\n", (u32)gettid());
	sprintf(log_msg,"[RC] thread start .....PID: %u\n", (u32)gettid());
	Write_Log(0,log_msg);
	int mkd;
    while(running) {

        // check fs stat for recycle
        if(!valid_path(rec_path)) {
            //ob_debug("%s not exist\n", rec_path);
			sprintf(log_msg,"[RC] %s not exist\n", rec_path);
			Write_Log(0,log_msg);
			if((mkd=mkdir("/mnt/data",S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))!=0)
			{
				Write_Log(0,"mkdir mnt/data fail\n");
			}
            continue;
        }

        struct statfs st;
        u32 r_free = 0;
        u32 r_size = 0;
        u32 r_used = 0;
        if(statfs(rec_path, &st) == 0) {
            r_free = (u32)(st.f_bfree / ((1024*1024)/st.f_bsize));
            r_size = (u32)(st.f_blocks / ((1024*1024)/st.f_bsize));
            r_used = (u32)((st.f_blocks - st.f_bavail) / ((1024*1024)/st.f_bsize));
            //ob_debug("Size=%uMB, Used=%uMB, Free=%uMB, Low Level=%uMB\n", r_size, r_used, r_free, min_space);
			sprintf(log_msg,"[RC] Size=%uMB, Used=%uMB, Free=%uMB, Low Level=%uMB\n", r_size, r_used, r_free, min_space);
			Write_Log(0,log_msg);
		}
        else {
            //ob_error("get fsstat failed: %s\n", strerror(errno));
			sprintf(log_msg,"[RC] get fsstat failed: %s\n", strerror(errno));
			Write_Log(0,log_msg);
		}

        find_time(rec_path, &rec_min_ts, &rec_min_ts2, &rec_max_ts);

        //ob_debug("Min: %u, Min2: %u, Max: %u\n", rec_min_ts, rec_min_ts2, rec_max_ts);
		sprintf(log_msg,"[RC] Min: %u, Min2: %u, Max: %u\n", rec_min_ts, rec_min_ts2, rec_max_ts);
		Write_Log(0,log_msg);

		if ((r_used/r_size)*100 > 90)
		{
			recycle(rec_path);
		}

        //if(r_free < min_space) {
        //    recycle(rec_path);
        //}

        sleep(60);

    }

    return 0;
}


// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------
int rec_init(rec_config_t *rc)
{
	char				log_msg[256];
    if(rc->ch_num <= 0)
        return -1;

    int i;
    char *buf = (char *)malloc(rc->ch_num*(OBE_PLATE_LEN+4));

    if(!buf)
        return -1;
    memset(buf, 0, (rc->ch_num*(OBE_PLATE_LEN+4)));
	get_ip("eth0",&my_ipaddress);
	//memset(temp_data, 0, 4000000);
    for(i=0;i<rc->ch_num;i++) {
        rec_t *r = (rec_t *)malloc(sizeof(rec_t));
        if(!r)
            break;
        memset(r, 0, sizeof(rec_t));
        r->rid = i;
        r->ip = rc->ch_info[i].ip;
		r->port = rc->ch_info[i].port;
        r->ch = rc->ch_info[i].ch;
        r->id = rc->ch_info[i].id;
		r->fps = rc->ch_info[i].fps;
		rec_fps[i] = rc->ch_info[i].fps;
		r->ch_index = i+1;
		sprintf(log_msg,"[TEST] alloc vs port = %d\n", r->port);
		Write_Log(0,log_msg);
        strncpy(r->path, rc->path, 63);
        if(r->ip)
            r->vs_handle = vsc_alloc(r->ip, r->ch, r->id, 0, r->port, rec_callback, r);
        else
            ;// local
		rc->ch_info[i].rec = r;
        if(!r->vs_handle) {
            //ob_error("[%d] alloc vs client failed\n", i);
			sprintf(log_msg,"[RC] [%d] alloc vs client failed\n", i);
			Write_Log(0,log_msg);
            free(r);
            continue;
        }

        r->magic = REC_MAGIC;
        plate_str[i] = buf + (i * (OBE_PLATE_LEN+4));
    }

    min_space = EST_SIZE * rc->ch_num;

    strncpy(rec_path, rc->path, 63);

    ch_nr = rc->ch_num;

    if(create_thread(rec_thread, 0) < 0)
        //ob_error("thread create failed\n");
		sprintf(log_msg,"[RC] thread create failed\n");
		Write_Log(0,log_msg);
    return 0;
}

int rec_exit(void)
{
    running = 0;
    return 0;
}

void rec_file_time(u32 sec, u32 *start, u32 *duration)
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

int rec_get_filename(int id, u32 ts, char *name, int ssize)
{
    char prefix[80];
    mk_filename(prefix, id, ts);
    snprintf(name, ssize, "%s/%s.ovs", rec_path, prefix);
    return 0;
}

void rec_next_timeslot(u32 sec, u32 *start, u32 *end)
{
    time_t ts = (time_t)(sec + TIME_SLOT);
    struct tm tt;
    localtime_r(&ts, &tt);
    tt.tm_min = tt.tm_sec = 0;
    time_t nts = mktime(&tt);
    if(start)
        *start = nts;
    if(end)
        *end = nts + TIME_SLOT - 1;
}

void rec_prev_timeslot(u32 sec, u32 *start, u32 *end)
{
    time_t ts = (time_t)(sec - TIME_SLOT);
    struct tm tt;
    localtime_r(&ts, &tt);
    tt.tm_min = tt.tm_sec = 0;
    time_t nts = mktime(&tt);
    if(start)
        *start = nts;
    if(end)
        *end = nts + TIME_SLOT - 1;
}

int rec_get_plate_filename(u32 ts, char *name, int ssize)
{
    char prefix[80];
    mk_prefix(prefix, ts);
    snprintf(name, ssize, "%s/%s.plate", rec_path, prefix);
    return 0;
}

int rec_get_vloss_filename(u32 ts, char *name, int ssize)
{
    char prefix[80];
    mk_prefix(prefix, ts);
    snprintf(name, ssize, "%s/%s.vloss", rec_path, prefix);
    return 0;
}

int rec_event_plate(u8 vs1_ch, u8 id, char *plate, u32 sec, u32 usec, u16 x, u16 y, u16 w, u16 h)
{
	char				log_msg[256];
    int retry = 0;
    int ret = -1;

    ob_event_t ev;
    ev.ch = vs1_ch;
    ev.id = id;
    ev.type = OBE_TYPE_PLATE;
    ev.sec = sec;
    ev.usec = usec;
    ev.x = x;
    ev.y = y;
    ev.width = w;
    ev.height = h;
    if(plate) {
        ev.flags = OBE_FLAG_CARIN;
        strncpy(ev.plate, plate, OBE_PLATE_LEN);
        strncpy(plate_str[vs1_ch], plate, OBE_PLATE_LEN);
    }
    else {
        ev.flags = 0;
#ifdef SAVE_PLATE
        strncpy(ev.plate, plate_str[vs1_ch], OBE_PLATE_LEN);
#else
        memset(ev.plate, 0, OBE_PLATE_LEN);
#endif
    }

    if(plate) {
        //ob_debug("[plate] CH%u %s (ON) @ %u\n", vs1_ch, ev.plate, sec);
		sprintf(log_msg,"[RC] [plate] CH%u %s (ON) @ %u\n", vs1_ch, ev.plate, sec);
		Write_Log(0,log_msg);
    }
    else {
        //ob_debug("[plate] CH%u (OFF) @ %u\n", vs1_ch, sec);
		sprintf(log_msg,"[RC] [plate] CH%u (OFF) @ %u\n", vs1_ch, sec);
		Write_Log(0,log_msg);
    }

    pthread_mutex_lock(&mutex_ev_p);

    do {
        if(ef_plate == 0) {
            if(!valid_path(rec_path))
                break;
            rec_get_plate_filename(sec, efp_filename, FN_SZ);
            u32 start, duration;
            rec_file_time(sec, &start, &duration);
            ef_plate = obe_open(efp_filename, 0, start, duration);
            if(ef_plate == 0) {
                //ob_error("[plate]%s open error\n", efp_filename);
				sprintf(log_msg,"[RC] [plate]%s open error\n", efp_filename);
				Write_Log(0,log_msg);
                break;
            }
            //ob_debug("[plate]%s opened : %u / %u\n", efp_filename, start, duration);
			sprintf(log_msg,"[RC] [plate]%s opened : %u / %u\n", efp_filename, start, duration);
			Write_Log(0,log_msg);
        }

        int wret = obe_write(ef_plate, &ev);
        if(wret > 0) {
            ret = 0;
            break;
        }
        else if(wret == 0) {
            obe_close(ef_plate);
            ef_plate = 0;
            retry++;
            continue;
        }
        else
            break;
    } while(retry < 2);

    pthread_mutex_unlock(&mutex_ev_p);

    return ret;
}

int rec_event_video(u8 ch, int vloss)
{
	char				log_msg[256];
    int retry = 0;
    int ret = -1;

    struct timeval ts;
    gettimeofday(&ts, 0);
    ob_event_t ev;
    ev.ch = ch;
    ev.type = (vloss) ? OBE_TYPE_VLOSS : OBE_TYPE_VRECOVERY;
    ev.flags = 0;
    ev.sec = (u32)ts.tv_sec;
    ev.usec = (u32)ts.tv_usec;

    if(vloss){
        //ob_debug("[vloss] CH%u VLOSS\n", ch);
		sprintf(log_msg,"[RC] [vloss] CH%u VLOSS\n", ch);
		Write_Log(0,log_msg);}
    else{
        //ob_debug("[vloss] CH%u VRECOVERY\n", ch);
		sprintf(log_msg,"[RC] [vloss] CH%u VRECOVERY\n", ch);
		Write_Log(0,log_msg);}

    pthread_mutex_lock(&mutex_ev_v);

    do {
        if(ef_video== 0) {
            if(!valid_path(rec_path))
                break;
            rec_get_vloss_filename(ev.sec, efv_filename, FN_SZ);
            u32 start, duration;
            rec_file_time(ev.sec, &start, &duration);
            ef_video = obe_open(efv_filename, 0, start, duration);
            if(ef_video == 0) {
                //ob_error("[vloss]%s open error\n", efv_filename);
				sprintf(log_msg,"[RC] [vloss]%s open error\n", efv_filename);
				Write_Log(0,log_msg);
                break;
            }
            //ob_debug("[vloss]%s opened : %u / %u\n", efv_filename, start, duration);
			sprintf(log_msg,"[RC] [vloss]%s opened : %u / %u\n", efv_filename, start, duration);
			Write_Log(0,log_msg);
        }

        int wret = obe_write(ef_video, &ev);
        if(wret > 0) {
            ret = 0;
            break;
        }
        else if(wret == 0) {
            obe_close(ef_video);
            ef_video = 0;
            retry++;
            continue;
        }
        else
            break;
    } while(retry < 2);

    pthread_mutex_unlock(&mutex_ev_v);

    return ret;
}

int rec_get_time(u32 *min, u32 *max)
{
    u32 t1, t2;
    find_time(rec_path, &t1, 0, &t2);
    if(t1 == 0xffffffff)
        return -1;
    if(t2 <= t1 || t2 == 0xffffffff)
        t2 = t1 + 3599;
    else
        t2 += 3599;
    if(min)
        *min = t1;
    if(max)
        *max = t2;
    return 0;
}


