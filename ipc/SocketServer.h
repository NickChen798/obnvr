
#ifndef _ScoketSERVER_H_
#define _ScoketSERVER_H_

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#include "../storage/ob_reader.h"
#include "../include/as_protocol.h"

#include "OBImgPrc.h"

#include "../dbg_msg/dbg_log.h"

#ifndef MAX_CH
#define MAX_CH		32
#endif

//typedef unsigned char       BYTE;
//typedef unsigned long       DWORD;
typedef int (*sock_callback_t) (BYTE *);

#ifndef u8
#define u8			unsigned __int8
#endif

#ifndef u32
#define u32			uint32_t
#endif

#ifndef GW_PORT
#define GW_PORT 8201
#endif

//#define msleep(n) usleep(n*1000)

typedef struct tagCMD_H{    
	DWORD	dwMagic;
	DWORD	dwVersion;
	DWORD	dwReserve;
	DWORD	dwLen;

} CMD_H;//, FAR *LPCMD_H;

typedef struct {
	CMD_H	h;
	char	*bufMessage;
} S_CMD;

#define CMD_H_SZ			sizeof(CMD_H)
#define	CMD_MAGIC			0x52564e69	//iNVR
#define	CMD_VERSION			0x20121001

typedef	struct
{
	int				ch_id;
	int				cmd_id;
	int				ev_type;
	int				date_type;
	int				option;

	time_t			time_start;
	struct timeval  millsec_start;
	time_t			time_end;
	struct timeval  millsec_end;

	char			cName[64];
	char			cFile[64];
	char			cEvent[64];
	char			cStrat_time[64];
	char			cEnd_time[64];
	char			cData_type[64];
	char			cOption[64];

}cmd_option;

typedef	struct
{
	int				worked;
	int				conn_client;

	cmd_option		cmd_info;
	int				img_type;

	char			msg[100000];

	//char			*plate;
	//BYTE			*img;
	rec_config_t	*reconfig;
	sock_callback_t sockback;

}socket_client;

typedef struct
{
	int				hSockSrv;
	int				port_num;
	int				lis_confirm;

	socket_client	client[10];

	//char			plate[plate_range];

	BYTE			*img_buf;

}socket_server;


//Api
void*	InitSocket(int port, sock_callback_t sockback, int option , rec_config_t *rc);
int		sock_start_listening(void* hSocket_server);
int		DestroySocket(void *hSocket_server);
int		Insert_Answer_List(int parking_id, char* plate, u32 timestamp, u32 micro_sec);
int		Insert_ch_list(int vs1_index, int ch_num, int tag);
int		Clean_Answer_List(int parking_id, u32 timestamp, u32 micro_sec);

void	*Create_client(u32 ip, int port, char *ip_loc);
void	Set_sck_param(void *gc, int park_id, int base_cout, int top_cout, result_info* ans, result_info *empty_ans, pthread_mutex_t *mux);
int		Gateway_Connect(void *hClient);
int		Gateway_Connect_Free(void *hClient);

int		Set_flag(void* gc, int park_id);
int		Set_max(void* gc, int park_id,int maxtype);
int		Set_Vsfail(int *state, int total_num);
extern int	Send_List(void *hClient, int ch_id, int base_cout, int *sort_list, int sort_max, void *result_list, void* empty_ans, char *sip, int opt);

#endif 