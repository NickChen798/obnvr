/*!
 *  @file       mcu.c
 *  @version    1.0
 *  @date       11/08/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       MCU Process
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "ob_platform.h"
#include "dbg_msg/dbg_log.h"
#include "mcu.h"
#include <termios.h>

// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "MCU"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------
#define CMD_SZ  32

#define TP2804_DEV_NONE			"/dev/ttyAMA1"

//command from TP2804 (Linux <-- 2804)
#define TPCMD_GET_PRODUCT_TYPE      0   //Bit0-3:Product Type, Bit4-7:Version Number
#define TPCMD_GET_ALMIN_01TO08      1
#define TPCMD_GET_ALMIN_09TO16      2
#define TPCMD_GET_VLOSS_01TO08      3
#define TPCMD_GET_VLOSS_09TO16      4
#define TPCMD_GET_FAN_FAIL          5
#define TPCMD_GET_FP_KEY            6   //Bit7:Hold Key, 0x00-0x2F:keyCode, 0x31-0x32:Jog, 0x41-0x4E:Shuttle,
#define TPCMD_GET_IR_KEY            7   //TDB: ask duncan
#define TPCMD_GET_KEYPAD_STATUS		8	// 0:ok, f:not ok

//command to TP2804 (Linux --> 2804)
#define TPCMD_SET_ALM_LED           0   //DATA Bit0-1:AlarmLED, Bit2-3:RecordLED (0=off, 1=on, 2=blink)
#define TPCMD_SET_CH_LED_01TO04     1   //DATA Bit0-1:CH1 LED, Bit2-3:CH2 LED, Bit4-5:CH3 LED, Bit6-7:CH4 LED (0=off, 1=on, 2=blink)
#define TPCMD_SET_CH_LED_05TO08     2   //DATA Bit0-1:CH5 LED, Bit2-3:CH6 LED, Bit4-5:CH7 LED, Bit6-7:CH8 LED (0=off, 1=on, 2=blink)
#define TPCMD_SET_CH_LED_09TO12     3   //DATA Bit0-1:CH9 LED, Bit2-3:CH10 LED, Bit4-5:CH11 LED, Bit6-7:CH12 LED (0=off, 1=on, 2=blink)
#define TPCMD_SET_CH_LED_13TO16     4   //DATA Bit0-1:CH13 LED, Bit2-3:CH14 LED, Bit4-5:CH15 LED, Bit6-7:CH16 LED (0=off, 1=on, 2=blink)
#define TPCMD_SET_ALMOUT            5   //DATA Bit0-3: AlarmOut1-4
#define TPCMD_SET_BUZZER            6   //DATA 0x01:ON,
#define TPCMD_SET_SHOW_STATUS       7   //DATA Bit0:Type&Version, Bit1:Fan, Bit2:AlarmIn, Bit3:VLoss
#define TPCMD_SET_WDT               8   //DATA 0xf0:disable, 0xf1..0xff => 2 .. 30 secs
#define TPCMD_SET_INIT_2804         9   //DATA 0xA5
#define TPCMD_SET_START_ISP         10  //DATA 0x5A
#define TPCMD_SET_KEYTONE           11  //DATA 0x00:disable key tone, 0x01:Enable key tone
#define TPCMD_SET_IRID              12  //DATA 1-9 ==> ID1 - ID9
#define TPCMD_POWEROFF   		    13

#define TPDATA_POWEROFF   		    0xaa

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------
typedef struct {

    int     fd;
    int     running;
    int     alarm_in;

    int     ridx;
    int     widx;
    u8      cmd[CMD_SZ];
    u8      data[CMD_SZ];

    mcu_callback_t cb;

    pthread_mutex_t mutex;
    char    node[128];

} mcu_t;

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------
static mcu_t    *tp2804 = 0;

//static char		log_msg[256];

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
static int dev_init(const char *node)
{
    int fd;
	struct termios com;
	int ret;
	char				log_msg[256];

	fd = open(node, O_RDWR);
	if(fd < 0) {
		sprintf(log_msg,"[MCU] open %s failed: %s(%d)\n", node, strerror(errno), errno);
		Write_Log(0,log_msg);
		//ob_error("open %s failed: %s(%d)\n", node, strerror(errno), errno);
		return -1;
	}

	memset(&com, 0, sizeof(struct termios));
	com.c_cflag |= (CLOCAL|CREAD);
	com.c_cflag &= ~CSIZE;
	com.c_cflag |= CS8;
	com.c_cflag &= ~(PARENB|PARODD);
	com.c_cflag &= ~CSTOPB;
	com.c_cflag &= ~CBAUD;
	com.c_cflag |= B57600;
	com.c_cflag &= ~CRTSCTS;
	ret = tcsetattr(fd, TCSANOW, &com);
	if (ret < 0) {
		//ob_error("tcsetattr error: %s(%d)\n", strerror(errno), errno);
		sprintf(log_msg,"[MCU] tcsetattr error: %s(%d)\n", strerror(errno), errno);
		Write_Log(0,log_msg);
        return -1;
	}

	return fd;
}

static int send_cmd(mcu_t *mc, u8 cmd, u8 data)
{
	u8  buf[2] = {0};
	int ret;
	char				log_msg[256];

	buf[0] = ((cmd << 4) & 0xf0) | ((data >> 4) & 0x0f);
	buf[1] = ((data << 4) & 0xf0) | ((~cmd) & 0x0f);

	ret = write(mc->fd, &buf[0], 1);
    if(ret != 1) {
        //ob_error("%s: write failed: %s\n", mc->node, strerror(errno));
		sprintf(log_msg,"[MCU] %s: write failed: %s\n", mc->node, strerror(errno));
		Write_Log(0,log_msg);
        return -1;
    }
	msleep(10);
	ret = write(mc->fd, &buf[1], 1);
    if(ret != 1) {
        //ob_error("%s: write failed: %s\n", mc->node, strerror(errno));
		sprintf(log_msg,"[MCU] %s: write failed: %s\n", mc->node, strerror(errno));
		Write_Log(0,log_msg);
        return -1;
    }
	msleep(10);

	return 0;
}

static int cmd_parser(mcu_t *mc, u8 d1, u8 d2)
{
	char				log_msg[256];
    if (((d1>>4)&0xf) != ((~d2)&0xf)) {
        //ob_error("%s: unknown cmd: %02x, %02x\n", mc->node, d1, d2);
		sprintf(log_msg,"[MCU] %s: unknown cmd: %02x, %02x\n", mc->node, d1, d2);
		Write_Log(0,log_msg);
        return -1;
    }

    u8 cmd  = (d1>>4)&0xf;
    u32 data = ((d1<<4)&0xf0) | ((d2>>4)&0xf);

    switch (cmd) {
        case TPCMD_GET_PRODUCT_TYPE:
            //ob_debug("GET_PRODUCT_TYPE: %02x\n", data);
			sprintf(log_msg,"[MCU] GET_PRODUCT_TYPE: %02x\n", data);
			Write_Log(0,log_msg);
            break;

        case TPCMD_GET_ALMIN_01TO08:
            //ob_debug("GET_ALMIN_01TO08: %02x\n", data);
			sprintf(log_msg,"[MCU] GET_ALMIN_01TO08: %02x\n", data);
			Write_Log(0,log_msg);
            mc->alarm_in = (mc->alarm_in & 0xff00) | data;
            if(mc->cb)
                (mc->cb)(MCU_EV_ALARMIN, mc->alarm_in);
            break;

        case TPCMD_GET_ALMIN_09TO16:
            mc->alarm_in = (mc->alarm_in & 0x00ff) | ((data<<8) & 0xff00);
            //ob_debug("GET_ALMIN_09TO16: %02x\n", data);
			sprintf(log_msg,"[MCU] GET_ALMIN_09TO16: %02x\n", data);
			Write_Log(0,log_msg);
            if(mc->cb)
                (mc->cb)(MCU_EV_ALARMIN, mc->alarm_in);
            break;

        case TPCMD_GET_FAN_FAIL:
            //ob_debug("GET_FAN_FAIL: %02x\n", data);
			sprintf(log_msg,"[MCU] GET_FAN_FAIL: %02x\n", data);
			Write_Log(0,log_msg);
            break;

        case TPCMD_GET_FP_KEY:
            //ob_debug("GET_FP_KEY: %02x\n", data);
			sprintf(log_msg,"[MCU] GET_FP_KEY: %02x\n", data);
			Write_Log(0,log_msg);
            if(mc->cb)
                (mc->cb)(MCU_EV_FP, data);
            break;

        case TPCMD_GET_IR_KEY:
            //ob_debug("GET_IR_KEY: %02x\n", data);
			sprintf(log_msg,"[MCU] GET_IR_KEY: %02x\n", data);
			Write_Log(0,log_msg);
            break;

        case TPCMD_GET_KEYPAD_STATUS:
            //ob_debug("GET_KETPAD_STATUS: %02x\n", data);
			sprintf(log_msg,"[MCU] GET_KETPAD_STATUS: %02x\n", data);
			Write_Log(0,log_msg);
            break;
    }
    return 0;
}

static void *mcu_thread(void *arg)
{
    mcu_t *mc = (mcu_t *)arg;
	char				log_msg[256];
    //ob_info("%s: thread start .....PID: %u\n", mc->node, (u32)gettid());
	sprintf(log_msg,"[MCU] %s: thread start .....PID: %u\n", mc->node, (u32)gettid());
	Write_Log(0,log_msg);

_mcu_init_:
    sleep(1);
    //ob_debug("%s: init\n", mc->node);
	sprintf(log_msg,"[MCU] %s: init\n", mc->node);
	Write_Log(0,log_msg);
	if(mc->fd >= 0) {
		close(mc->fd);
		mc->fd = -1;
	}

    mc->fd = dev_init(mc->node);
    if(mc->fd < 0) {
        //ob_error("%s: init failed\n", mc->node);
		sprintf(log_msg,"[MCU] %s: init failed\n", mc->node);
		Write_Log(0,log_msg);
        goto _mcu_init_;
    }
    else {
        u8 cbuf[3] = {0xff, 0x9A, 0x56};
        //ob_debug("%s: opened\n", mc->node);
		sprintf(log_msg,"[MCU] %s: opened\n", mc->node);
		Write_Log(0,log_msg);
        msleep(10);
        int wret = write(mc->fd, cbuf, 1);
        msleep(10);
        wret = write(mc->fd, cbuf+1, 1);
        msleep(10);
        wret = write(mc->fd, cbuf+2, 1);
        if(wret != 1) {
            //ob_error("%s: write init cmd failed\n", mc->node);
			sprintf(log_msg,"[MCU] %s: write init cmd failed\n", mc->node);
			Write_Log(0,log_msg);
            goto _mcu_init_;
        }
	send_cmd(mc, TPCMD_SET_SHOW_STATUS, 0);
    }

    //ob_debug("%s: init done\n", mc->node);
	sprintf(log_msg,"[MCU] %s: init done\n", mc->node);
	Write_Log(0,log_msg);

	while(mc->running) {

        fd_set rds, wds;
        struct timeval tv;
        int retval;

        FD_ZERO(&rds);
        FD_SET(mc->fd, &rds);
        FD_ZERO(&wds);
        FD_SET(mc->fd, &wds);

        tv.tv_sec = 0;
        tv.tv_usec = 50000;

        retval = select(mc->fd+1, &rds, &wds, 0, &tv);

        if(retval < 0) {
            if(errno != EINTR) {
                //ob_error("%s: select error: %s\n", mc->node, strerror(errno));
				sprintf(log_msg,"[MCU] %s: select error: %s\n", mc->node, strerror(errno));
				Write_Log(0,log_msg);
                goto _mcu_init_;
            }
            continue;
        }
        if(retval == 0)
		{
			sprintf(log_msg,"[MCU] %s: select retval = 0: %s\n", mc->node, strerror(errno));
			Write_Log(0,log_msg);
			continue;
		}
        if(FD_ISSET(mc->fd, &rds)) {
            char rbuf[2];
            int ret = read(mc->fd, rbuf, 2);
            if(ret != 2) {
                if(ret < 0) {
                    //ob_error("%s: read error: %s\n", mc->node, strerror(errno));
					sprintf(log_msg,"[MCU] %s: read error: %s\n", mc->node, strerror(errno));
					Write_Log(0,log_msg);
                }
                else {
                    //ob_error("%s: read %d (need 2)\n", mc->node, ret);
					sprintf(log_msg,"[MCU] %s: read %d (need 2)\n", mc->node, ret);
					Write_Log(0,log_msg);
                }
                goto _mcu_init_;
            }
            cmd_parser(mc, rbuf[0], rbuf[1]);
        }

        if(FD_ISSET(mc->fd, &wds)) {
            if(mc->ridx != mc->widx) {
		    //ob_debug("%s: write cmd @ RW=%d/%d (c:%02x, d:%02x)\n",
            //                mc->node, mc->ridx, mc->widx, mc->cmd[mc->ridx], mc->data[mc->ridx]);
			sprintf(log_msg,"[MCU] %s: write cmd @ RW=%d/%d (c:%02x, d:%02x)\n",
                            mc->node, mc->ridx, mc->widx, mc->cmd[mc->ridx], mc->data[mc->ridx]);
			Write_Log(0,log_msg);
                if(send_cmd(mc, mc->cmd[mc->ridx], mc->data[mc->ridx]) < 0) {
                    //ob_error("%s: write cmd failed @ RW=%d/%d (c:%02x, d:%02x)\n",
                    //        mc->node, mc->ridx, mc->widx, mc->cmd[mc->ridx], mc->data[mc->ridx]);
					sprintf(log_msg,"[MCU] %s: write cmd failed @ RW=%d/%d (c:%02x, d:%02x)\n",
                            mc->node, mc->ridx, mc->widx, mc->cmd[mc->ridx], mc->data[mc->ridx]);
					Write_Log(0,log_msg);
                }
                else {
                    mc->ridx = (mc->ridx + 1) % CMD_SZ;
                }
            }
            else {
                msleep(10);
                continue;
            }
        }
    }

//_mcu_exit_:
    if(mc->fd >= 0) {
        close(mc->fd);
        mc->fd = -1;
    }
	//ob_error("%s: thread stopped\n", mc->node);
	sprintf(log_msg,"[MCU] %s: thread stopped\n", mc->node);
	Write_Log(0,log_msg);
    free(mc);
    return 0;
}

static int enqueue(mcu_t *mc, u8 cmd, u8 data)
{
	char				log_msg[256];
    if(mc == 0 || !mc->running)
        return -1;

	pthread_mutex_lock(&mc->mutex);
    int next = (mc->widx + 1) % CMD_SZ;
    if(next == mc->ridx) {
        pthread_mutex_unlock(&mc->mutex);
        //ob_error("%s: cmd queue full: R/W=%d/%d\n", mc->node, mc->ridx, mc->widx);
		sprintf(log_msg,"[MCU] %s: cmd queue full: R/W=%d/%d\n", mc->node, mc->ridx, mc->widx);
		Write_Log(0,log_msg);
        return -1;
    }
    mc->cmd[mc->widx] = cmd;
    mc->data[mc->widx] = data;
    mc->widx = next;

	pthread_mutex_unlock(&mc->mutex);

    return 0;
}

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------
int mcu_init(mcu_callback_t cb)
{
	char				log_msg[256];
    if(tp2804)
        return 0;

    tp2804 = (mcu_t *)malloc(sizeof(mcu_t));
    if(tp2804 == 0) {
        //ob_error("OOM\n");
		Write_Log(0,"OOM\n");
        return -1;
    }
    memset(tp2804, 0, sizeof(mcu_t));
    tp2804->fd = -1;
    pthread_mutex_init(&tp2804->mutex, 0);
    strcpy(tp2804->node, TP2804_DEV_NONE);
    tp2804->cb = cb;
    tp2804->running = 1;
    if(create_thread(mcu_thread, tp2804) < 0) {
        //ob_error("thread create failed\n");
		sprintf(log_msg,"[MCU] thread create failed\n");
		Write_Log(0,log_msg);
        free(tp2804);
        tp2804 = 0;
        return -1;
    }
    return 0;
}

int mcu_exit(void)
{
    if(tp2804) {
        tp2804->running = 0;
        msleep(50);
    }
    return 0;
}

int mcu_alarmout(u8 data)
{
    return enqueue(tp2804, TPCMD_SET_ALMOUT, data);
}

int mcu_buzzer(int on)
{
    return enqueue(tp2804, TPCMD_SET_BUZZER, (on) ? 0x01 : 0x00);
}

int mcu_wdt_reset(int sec)
{
    if(sec > 255)
        sec = 255;
    if(sec < 1)
        sec = 1;
    return enqueue(tp2804, TPCMD_SET_WDT, (u8)sec);
}

int mcu_wdt_disable(void)
{
    return enqueue(tp2804, TPCMD_SET_WDT, 0);
}

int mcu_poweroff(void)
{
    return enqueue(tp2804, TPCMD_POWEROFF, TPDATA_POWEROFF);
}

