/*!
 *  @file       vsc.c
 *  @version    1.0
 *  @date       18/03/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       VS1 Client <br>
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "ob_platform.h"
#include "vsc.h"
#include "dbg_msg/dbg_log.h"

// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "VSC"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------
#define VSC_MAGIC   0x7653634d      // 'vScM'
#define BSIZE       0x300000

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------
typedef struct {

    u32     magic;
    u32     ip;
	int		port;
    u8      ch;
    u8      id;
	u8		ch_index;
    u8      mode;       // 0=normal, 1=Y only

    u8      flags;
    u8      *rbuf;
    u32     bsize;

    void    *priv;
    vsc_callback_t cb;

} vsc_t;

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------

//static char		log_msg[256];
static int			vsf[MAX_CH+1];
int			totalvs;

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
static void *vsc_thread(void *arg)
{
	char		log_msg[256];
    vsc_t *vc = (vsc_t *)arg;
	int sd = -1;
	struct timeval tt;
	struct timeval rto;

	u32 now;
	u32 aft;

    //ob_info("<%08x>thread start .....PID: %u\n", vc->ip, (u32)gettid());
	sprintf(log_msg,"[VSC] [%d] <%08x>thread start .....PID: %u vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip, (u32)gettid(), vc->ch , vsf[vc->ch]);
	Write_Log(0,log_msg);

_vsc_init_:
	if(sd >= 0) {
		close(sd);
		sd = -1;
		if(vc->cb && !vc->mode) // vloss
		{			
			(vc->cb)(vc->priv, 0, 0);
		}
	}

    if(!vc->ip) {
        //ob_error("<%08x>invalid ip\n", vc->ip);
		sprintf(log_msg,"[VSC] [%d] <%08x>invalid ip vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip,vc->ch,vsf[vc->ch]);
		Write_Log(0,log_msg);
		vsf[vc->ch] = -1;
        goto _vsc_exit_;
    }

    //ob_debug("<%08x>connecting\n", vc->ip);
	sprintf(log_msg,"[VSC] [%d] <%08x> [%d] connecting vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip , vc->mode , vc->ch, vsf[vc->ch]);
	Write_Log(0,log_msg);

    sd = socket(PF_INET, SOCK_STREAM, 0);
    if(sd < 0) {
        //ob_error("<%08x>socket error: %s", vc->ip, strerror(errno));
		sprintf(log_msg,"[VSC] [%d] <%08x>socket error: %s vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip, strerror(errno), vc->ch, vsf[vc->ch]);
		Write_Log(0,log_msg);
		vsf[vc->ch] = -1;
        goto _vsc_exit_;
    }
	//sprintf(log_msg,"[VSC] [ROITEST] port = %d vc->ch = %d\n", vc->port,vc->ch);
	//Write_Log(0,log_msg);
	struct sockaddr_in  peer;
    peer.sin_family = PF_INET;
    peer.sin_port = htons(vc->port);
    peer.sin_addr.s_addr = htonl(vc->ip);
    while(connect(sd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
        //ob_error("<%08x>connect error: %s\n", vc->ip, strerror(errno));
		sprintf(log_msg,"[VSC] [%d] <%08x>connect error: %s vc_ch = %d vsf[vc->ch] = %d %u\n", vc->mode, vc->ip, strerror(errno), vc->ch, vsf[vc->ch],vc->port);
		Write_Log(0,log_msg);
		vsf[vc->ch] = -1;
        if(!vc->ip || !vc->magic)
            goto _vsc_exit_;
		msleep(1000);
    }

	rto.tv_sec = 2;  /* 30 Secs Timeout */
	rto.tv_usec = 0;  // Not init'ing this can cause strange errors

	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&rto,sizeof(struct timeval));

    as_cmd_t s_cmd;
    s_cmd.magic = ASCMD_MAGIC;
    s_cmd.version = ASCMD_VERSION;
    if(!vc->mode) {
        s_cmd.cmd = ASCMD_S2D_STREAM;
        s_cmd.flag = ASCMD_S2D_STREAM_VIDEO | ((vc->ch<<4)&0xf0) | (vc->id&0x0f);
    }
    else {
        s_cmd.cmd = ASCMD_S2D_ROI_MODE;
        s_cmd.flag = 0;
    }
	sprintf(log_msg,"[VSC] [%d] s_cmd.flag = %d \n", vc->mode, s_cmd.flag);
	Write_Log(0,log_msg);
    s_cmd.len = sizeof(as_cmd_t);
    int ret = send(sd, (char *)&s_cmd, s_cmd.len, 0);
    if(ret != s_cmd.len) {
        //ob_error("<%08x>send command faild: %s\n", vc->ip, strerror(errno));
		sprintf(log_msg,"[VSC] [%d] <%08x>send command faild: %s vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip, strerror(errno), vc->ch, vsf[vc->ch]);
		Write_Log(0,log_msg);
		vsf[vc->ch] = -1;
        goto _vsc_init_;
    }

	gettimeofday(&tt, 0);
	aft = (u32) tt.tv_sec;

	while(vc->magic == VSC_MAGIC) {

        fd_set rds;
        struct timeval tv;
        int retval;

        FD_ZERO(&rds);
        FD_SET(sd, &rds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        retval = select(sd+1, &rds, 0, 0, &tv);
        if(retval < 0) {
            if(errno != EINTR) {
                //ob_error("<%08x>select error: %s\n", vc->ip, strerror(errno));
				sprintf(log_msg,"[VSC] [%d] <%08x>select error: %s vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip, strerror(errno), vc->ch, vsf[vc->ch]);
				Write_Log(0,log_msg);
				vsf[vc->ch] = -1;
                goto _vsc_init_;
            }
            continue;
        }
        else if(retval == 0 || !FD_ISSET(sd, &rds))
			{
				gettimeofday(&tt, 0);
				now = (u32) tt.tv_sec;
				if ((now-aft) > 120)
				{
					vsf[vc->ch] = -1;
					sprintf(log_msg,"[VSC] <%08x> [%d] over time: now = %d aft = %d vc_ch = %d vsf[vc->ch] = %d\n", vc->ip, vc->mode, now , aft , vc->ch, vsf[vc->ch]);
					Write_Log(0,log_msg);
					goto _vsc_init_;
				}
				continue;
			}
		//sprintf(log_msg,"[TEST] <%08x> [%d] vc->cb : %X  vc_ch = %d vsf[vc->ch] = %d\n", vc->ip, vc->mode, vc->cb, vc->ch, vsf[vc->ch]);
		//Write_Log(0,log_msg);
        if(vc->mode == 0) {
            as_frame_t *fh = (as_frame_t *)vc->rbuf;
            int rsize = 0;
            int off = 0;
            fh->magic = 0;

			gettimeofday(&tt, 0);
			aft = (u32) tt.tv_sec;
			vsf[vc->ch_index] = 0;

            do {
                ret = recv(sd, (char *)vc->rbuf+off, sizeof(as_frame_t)-rsize, 0);
                if(ret <= 0) {
                    //ob_debug("<%08x> %d/%d \n", vc->ip,log_msgs;ze);
					sprintf(log_msg,"[VSC] [%d] [ERROR] recv failed<%08x> %d/%d vc_ch=%d vsf[vc->ch] = %d\n", vc->mode, vc->ip, ret, rsize , vc->ch, vsf[vc->ch_index]);
					Write_Log(0,log_msg);
					vsf[vc->ch_index] = -1;
                    goto _vsc_init_;
                }
                rsize += ret;
                off += ret;
            } while(rsize < sizeof(as_frame_t));

            if(fh->magic != ASF_MAGIC) {
                //ob_error("<%08x>wrong magic %x != %x\n", vc->ip, fh->magic, ASF_MAGIC);
				sprintf(log_msg,"[VSC] [%d] [ERROR] <%08x> wrong magic %x != %x vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip, fh->magic, ASF_MAGIC, vc->ch, vsf[vc->ch_index]);
				Write_Log(0,log_msg);
				vsf[vc->ch_index] = -1;
                goto _vsc_init_;
            }

            if(fh->size > vc->bsize) {
                //ob_error("<%08x>oversized frame :%u\n", vc->ip, fh->size);
				sprintf(log_msg,"[VSC] [%d] [ERROR] <%08x> oversized frame :%u vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip, fh->size, vc->ch, vsf[vc->ch_index]);
				Write_Log(0,log_msg);
				vsf[vc->ch_index] = -1;
                goto _vsc_init_;
            }

			if(fh->size) {						
                rsize = 0;
                off = sizeof(as_frame_t);
                do {
                    ret = recv(sd, (char *)vc->rbuf+off, fh->size-rsize, 0);
                    if(ret <= 0) {
                        //ob_error("<%08x>recv faild: %s\n", vc->ip, strerror(errno));
						sprintf(log_msg,"[VSC] [%d] [ERROR] <%08x> recv faild: %s vc_ch = %d vsf[vc->ch] = %d\n", vc->mode, vc->ip, strerror(errno), vc->ch ,vsf[vc->ch_index]);
						Write_Log(0,log_msg);
						vsf[vc->ch_index] = -1;
                        goto _vsc_init_;
                    }
                    rsize += ret;
                    off += ret;
                } while(rsize < (int)fh->size);
            }
			//sprintf(log_msg,"[TEST] [%d] <%08x> vc->cb : %X vsf[vc->ch] = %d vc_ch = %d\n", vc->mode, vc->ip , vc->cb , vsf[vc->ch], vc->ch);
			//Write_Log(0,log_msg);
            if(vc->cb) {
                (vc->cb)(vc->priv, fh, (fh->size) ? (vc->rbuf+sizeof(as_frame_t)) : 0);
            }
        }
        else { // Y DATA
            as_yframe_t *fh = (as_yframe_t *)vc->rbuf;
            int rsize = 0;
            int off = 0;
            fh->magic = 0;

			gettimeofday(&tt, 0);
			aft = (u32) tt.tv_sec;
			vsf[vc->ch] = 0;

            do {
                ret = recv(sd, (char *)vc->rbuf+off, sizeof(as_yframe_t)-rsize, 0);
                if(ret <= 0) {
                    //ob_debug("<%08x> %d/%d \n", vc->ip, ret, rsize);
					sprintf(log_msg,"[VSC] [Y]<%08x> %d/%d vc_ch = %d vsf[vc->ch] = %d\n", vc->ip, ret, rsize, vc->ch,vsf[vc->ch]);
					Write_Log(0,log_msg);
					vsf[vc->ch] = -1;
                    goto _vsc_init_;
                }
                rsize += ret;
                off += ret;
            } while(rsize < sizeof(as_yframe_t));

            if(fh->magic != ASYF_MAGIC) {
                //ob_error("<%08x>wrong magic %x != %x\n", vc->ip, fh->magic, ASYF_MAGIC);
				sprintf(log_msg,"[VSC] [Y]<%08x>wrong magic %x != %x vc_ch = %d vsf[vc->ch] = %d\n", vc->ip, fh->magic, ASYF_MAGIC, vc->ch, vsf[vc->ch]);
				Write_Log(0,log_msg);
				vsf[vc->ch] = -1;
                goto _vsc_init_;
            }

            if(fh->size > vc->bsize) {
                //ob_error("<%08x>oversized frame :%u\n", vc->ip, fh->size);
				sprintf(log_msg,"[VSC] [Y]<%08x>oversized frame :%u vc_ch = %d vsf[vc->ch] = %d\n", vc->ip, fh->size,vc->ch, vsf[vc->ch]);
				Write_Log(0,log_msg);
				vsf[vc->ch] = -1;
                goto _vsc_init_;
            }

            if(fh->size) {
                rsize = 0;
                off = sizeof(as_yframe_t);
                do {
					//if (vc->rbuf!=NULL)
					//{
					//	Write_Log(0,"rbuf != NULL\n");						
					//}
					
                    ret = recv(sd, (char *)vc->rbuf+off, fh->size-rsize, 0);
                    if(ret <= 0) {
                        //ob_error("<%08x>recv faild: %s\n", vc->ip, strerror(errno));
						sprintf(log_msg,"[VSC] [Y]<%08x>recv faild: %s vc_ch = %d vsf[vc->ch] = %d\n", vc->ip, strerror(errno),vc->ch, vsf[vc->ch]);
						Write_Log(0,log_msg);
						vsf[vc->ch] = -1;
                        goto _vsc_init_;
                    }
                    rsize += ret;
                    off += ret;
                } while(rsize < (int)fh->size);
            }

            if(vc->cb) {
                (vc->cb)(vc->priv, fh, (fh->size) ? (vc->rbuf+sizeof(as_yframe_t)) : 0);
            }
        }
		//vsf[vc->ch] = 0;
		//sprintf(log_msg,"[TEST] [While_End] [%d] <%08x> vc->cb : %X vsf[vc->ch] = %d vc_ch = %d\n",vc->mode , vc->ip , vc->cb,vsf[vc->ch], vc->ch);
		//Write_Log(0,log_msg);
    }

_vsc_exit_:
    if(sd >= 0)
        close(sd);
    vc->ip = 0;
	vsf[vc->ch] = -1;
	//ob_error("<%08x>thread stopped\n", vc->ip);
	sprintf(log_msg,"[VSC] <%08x>thread stopped.\n");
	Write_Log(0,log_msg);
    while(vc->magic == VSC_MAGIC)
		msleep(100);
    free(vc);
	//ob_error("<%08x>thread closed\n", vc->ip);
	sprintf(log_msg,"[VSC] <%08x>thread closed.\n");
	Write_Log(0,log_msg);
    return 0;
}

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------
void *vsc_alloc(u32 ip, u8 ch, u8 id, u8 mode, int port, vsc_callback_t cb, void *priv)
{
	char		log_msg[256];
    vsc_t *vc = (vsc_t *)malloc(sizeof(vsc_t)+BSIZE);
    if(!vc)
        return 0;
    memset(vc, 0, sizeof(vsc_t));
    vc->rbuf = ((u8 *)vc) + sizeof(vsc_t);
    vc->bsize = BSIZE;
    vc->ip = ip;
	vc->port = port;
    vc->ch = ch;
    vc->id = id;
    vc->mode = mode;
    vc->priv = priv;
    vc->cb = cb;
    vc->magic = VSC_MAGIC;
	totalvs = ch;
    if(create_thread(vsc_thread, vc) < 0) {
        //ob_error("<%08x>thread create failed\n", ip);
		sprintf(log_msg,"[VSC] <%08x>thread create failed\n", ip);
		Write_Log(0,log_msg);
        free(vc);
        return 0;
    }
    return vc;
}

int vsc_free(void *handle)
{
    vsc_t *vc = (vsc_t *)handle;
    if(vc->magic != VSC_MAGIC)
        return -1;

    vc->magic = 0;
    return 0;
}

void Ask_Vs_State(int *Vs_state)
{
	char		log_msg[64];
	
	Vs_state[0] =	totalvs;
	
	int z;
	for (z = 1;z<totalvs+1 ; z++)
	{
		Vs_state[z] = vsf[z];
		sprintf(log_msg,"[VSC] Ask_Vs_State = %d\n", vsf[z]);
		Write_Log(0,log_msg);
	}	
	
	/*if (vsf)
	{
		return vsf; 
	} 
	else
	{
		return 0;
	}*/
}

