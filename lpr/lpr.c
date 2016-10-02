/*!
 *  @file       lpr.c
 *  @version    1.0
 *  @date       27/03/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       LPR APIs
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "OBWinDef.h"
#include "OBErrMsg.h"
#include "OBLogFil.h"
#include "OBImgPrc.h"
#include "CVWnLPR.h"
#include "CP_LprFrm.h"
#include "lpr.h"
#include "ipc/SocketServer.h"
#include "dbg_msg/dbg_log.h"

// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "LPR"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"
int		dedug_flag ;

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------
#define LPR_MAGIC       0x4f6c5072      // 'OlPr'
#define DEFAULT_WIDTH   320
#define DEFAULT_HEIGHT  240

#define BSIZE			0x500000

//#define MAX_CH		32
// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------
 typedef	struct
 {
 	char	plate[32];
 	int		cout;

	int		x1;
	int		y1;
	int		x2;
	int		y2;

 	//BYTE	imgbuf[76800];
 }lpr_result;

typedef struct {

    u32     magic;
	DWORD	dwRegion;
    HANDLE  hLic;
    HANDLE  hLpr;
	HIMAGE	hImage;

	int		park_id;
	
	//lpr param
    int     width;
    int     height;
	int     format;
	int		loc_more;
	int		loc_type;
	int		loc_top;
	int		loc_btm;
	int		loc_left;
	int		loc_right;
	int		plate_width;
	int		plate_height;
	int		threshold;
	int		edge;
	int		nIndex;
	int		excp_lv;
	int		base_number;
	int		top_number;

    void    *extPtr;
    char    Msg[MAX_PATH];
	PICRST	PicRst;

	result_info *ans_list;

    HLOG    hLog;

} lpr_ctrl_t;

//result_info tmp_del[MAX_CH];

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------

//static char				log_msg[256];

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
void lpr_free(void *handle)
{
	char				log_msg[256];
    lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;

    if(lc->magic != LPR_MAGIC) {
		sprintf(log_msg,"[LPR] [FREE] Wrong MAGIC\n");
		Write_Log(0,log_msg);
        //ob_error("Wrong MAGIC\n");
        return ;
    }

    if(lc->hLog) {
		sprintf(log_msg,"[LPR] [FREE] [ch%04d] free lpr log", lc->park_id );
		Write_Log(0,log_msg);
        FreeLog(lc->hLog);
        lc->hLog = 0;
    }

    if(lc->hImage) {
        if(lc->extPtr) {
			sprintf(log_msg,"[LPR] [FREE] [ch%04d] unhook lpr himage" , lc->park_id);
			Write_Log(0,log_msg);
            ImageUnhookBuffer(lc->hImage);
            // free buffer here ?
        }
        OCR_FreeImage(lc->hImage);
        lc->hImage = 0;
    }

    if(lc->hLic) {
        if(!WNLPRClose(lc->hLic))
		{
			sprintf(log_msg,"[LPR] [FREE] [ch%04d] free lpr " , lc->park_id);
			Write_Log(0,log_msg);
			//ShowError();
		}
        lc->hLic = NULL;
    }
    lc->magic = 0;
}

int lpr_param_read(void *lpr_handle, int parking_id, char *filename)
{
	char				log_msg[256];
	lpr_ctrl_t *lc = (lpr_ctrl_t *)lpr_handle;

	char		section[32];
	char		tmp[32];

	//printf("ch[%d] - read ini lpr .\n", parking_id);
	sprintf(log_msg,"[LPR] [Read] ch[%d] - read ini lpr .\n", parking_id);
	Write_Log(0,log_msg);

	sprintf(section, "//mnt//dbg//test%d.log", lc->park_id);
	lc->hLog = CreateLog(section);

	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Top_No", "100", tmp, 32, filename );
	lc->top_number = atoi(tmp);
	GetPrivateProfileString( section, "Base_No", "5", tmp, 32, filename );
	lc->base_number = atoi(tmp);
 	GetPrivateProfileString( section, "Region", "1", tmp, 32, filename );
 	lc->dwRegion = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Plate_Format", "17", tmp, 32, filename );
	lc->format = atoi(tmp);
	sprintf(log_msg,"[LPR] [Read] ch[%d] - region=%d, format=%d .\n", parking_id, lc->dwRegion, lc->format);
	Write_Log(0,log_msg);

	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Img_Width", "320", tmp, 32, filename );
	lc->width = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Img_Height", "240", tmp, 32, filename );
	lc->height = atoi(tmp);
	sprintf(log_msg,"[LPR] [Read] ch[%d] - img_w=%d, img_h=%d .\n", parking_id, lc->width, lc->height);
	Write_Log(0,log_msg);

	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Loc_Method", "6", tmp, 32, filename );
	lc->loc_type = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Loc_Number", "6", tmp, 32, filename );
	lc->loc_more = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Loc_Top", "0", tmp, 32, filename );
	lc->loc_top = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Loc_Bottom", "239", tmp, 32, filename );
	lc->loc_btm = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Loc_Left", "0", tmp, 32, filename );
	lc->loc_left = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Loc_Right", "319", tmp, 32, filename );
	lc->loc_right = atoi(tmp);
	sprintf(log_msg,"[LPR] [Read] ch[%d] - loc M=%d, N=%d, T=%d, B=%d, L=%d, R=%d .\n", parking_id, lc->loc_type, lc->loc_more, lc->loc_top, lc->loc_btm, lc->loc_left, lc->loc_right);
	Write_Log(0,log_msg);
	
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Plate_Width", "110", tmp, 32, filename );
	lc->plate_width = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Plate_Height", "50", tmp, 32, filename );
	lc->plate_height = atoi(tmp);
	sprintf(log_msg,"[LPR] [Read] ch[%d] - plate W=%d, H=%d .\n", parking_id, lc->plate_width, lc->plate_height);
	Write_Log(0,log_msg);

	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Threshold", "-35", tmp, 32, filename );
	lc->threshold = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Edge", "8", tmp, 32, filename );
	lc->edge = atoi(tmp);
	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "nIindex", "110", tmp, 32, filename );
	lc->nIndex = atoi(tmp);

	GetPrivateProfileString( "System", "Debug_Mod", "0", tmp, 32, filename );
	dedug_flag = atoi(tmp);

	sprintf(section, "Parking%d", parking_id);
	GetPrivateProfileString( section, "Excp_Lv", "9", tmp, 32, filename );
	lc->excp_lv = atoi(tmp);
	sprintf(log_msg,"[LPR] [Read] ch[%d] - others T=%d, E=%d I=%d, Excp=%d .\n", parking_id, lc->threshold, lc->edge, lc->nIndex, lc->excp_lv);
	Write_Log(0,log_msg);
	
	if(lc->hLpr !=0)
	{//set
		sprintf( lc->Msg, "[LPR] [Read] ch[%d] %d", lc->edge, parking_id );
		if( !WNLPRSetParam(lc->hLpr, LPR_PRM_EDGE, lc->Msg) )
			Write_Log(0,lc->Msg);//ShowError();
		sprintf( lc->Msg, "[LPR] [Read] ch[%d] %d", lc->loc_type, parking_id );
		if( !WNLPRSetParam(lc->hLpr, LPR_PRM_LOCMTHD, lc->Msg) )
			Write_Log(0,lc->Msg);//ShowError();
		//sprintf( lc->Msg, "%d", LPR_PRM_TWN_FMT_CAR|LPR_PRM_TWN_FMT_PTAXI );
		sprintf( lc->Msg, "[LPR] [Read] ch[%d] %d", lc->format, parking_id );
		if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PFORMAT, lc->Msg) )
			Write_Log(0,lc->Msg);//ShowError();
		sprintf( lc->Msg, "[LPR] [Read] ch[%d] %d", lc->excp_lv, parking_id );
		if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PEXPLVL, lc->Msg) )
			Write_Log(0,lc->Msg);//ShowError();
		sprintf( lc->Msg, "[LPR] [Read] ch[%d] %d", lc->loc_more, parking_id );
		if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PLATENO, lc->Msg) )
			Write_Log(0,lc->Msg);//ShowError();
		sprintf( lc->Msg, "[LPR] [Read] ch[%d] %d", lc->plate_width, parking_id );
		if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PWIDTH, "110") )
			Write_Log(0,lc->Msg);//ShowError();
		sprintf( lc->Msg, "[LPR] [Read] ch[%d] %d", lc->plate_height, parking_id );
		if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PHEIGHT, "50") )
			Write_Log(0,lc->Msg);//ShowError();
	}

	return 1;
}

void *lpr_alloc(int parking_id, result_info* result, char *filename)
{
	char				log_msg[256];
    lpr_ctrl_t *lc = (lpr_ctrl_t *)malloc(sizeof(lpr_ctrl_t));
    if(!lc) {
        //ob_error("
		sprintf(log_msg,"[LPR] [Alloc] Out of memory\n");
		Write_Log(0,log_msg);
        return 0;
    }

	sprintf(log_msg,"[LPR] [Alloc] ch[%d] - initEx lpr ,%s \n", parking_id, filename);
	Write_Log(0,log_msg);
    memset(lc, 0, sizeof(lpr_ctrl_t));
    lc->magic = LPR_MAGIC;
	lc->park_id = parking_id;
    
	//get from ini file
	lc->dwRegion = 1;

	lpr_param_read(lc, parking_id, filename);
		
    lc->hLic = WNLPRInitEx(LPR_LIC_CR20, NULL, lc->dwRegion);
	if(lc->hLic == NULL) {
		//ShowError();
		sprintf(log_msg,"[LPR] [Alloc] [ERROR] [ch%04d] WNLPRInitEx fail.\n", lc->park_id);
		Write_Log(0,log_msg);
        lpr_free(lc);
		return 0;
	}

    //lc->hLog = CreateLog("./obnvr.log");
    //ob_debug("hLog = %p\n", lc->hLog);

	sprintf(log_msg,"[LPR] [Alloc] [[ch%04d]] create lpr w=%d, h=%d .\n", parking_id, lc->width, lc->height);
	Write_Log(0,log_msg);    
	int nStatus;
    //lc->hLpr = WNLPRCreateEx2(lc->hLic, lc->width, lc->height, "./", &nStatus, NULL, lc->hLog);
	lc->hLpr = WNLPRCreateEx2(lc->hLic, 1280, 720, "./", &nStatus, NULL, lc->hLog);
    if(lc->hLpr == NULL) {
		sprintf(log_msg,"[LPR] [Alloc] [ERROR] [ch%04d] WNLPRCreateEx2 fail.\n", lc->park_id);
		Write_Log(0,log_msg);
		//ShowError();
        lpr_free(lc);
		return 0;
	}

	//set param 
	sprintf( lc->Msg, "%d", lc->edge );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_EDGE, lc->Msg) )
		Write_Log(0,lc->Msg);
	sprintf( lc->Msg, "%d", lc->loc_type );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_LOCMTHD, lc->Msg) )
		Write_Log(0,lc->Msg);
	//sprintf( lc->Msg, "%d", LPR_PRM_TWN_FMT_CAR|LPR_PRM_TWN_FMT_PTAXI );
	sprintf( lc->Msg, "%d", lc->format );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PFORMAT, lc->Msg) )
		Write_Log(0,lc->Msg);
	sprintf( lc->Msg, "%d", lc->excp_lv );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PEXPLVL, lc->Msg) )
		Write_Log(0,lc->Msg);
	sprintf( lc->Msg, "%d", lc->loc_more );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PLATENO, lc->Msg) )
		Write_Log(0,lc->Msg);
	sprintf( lc->Msg, "%d", lc->plate_width );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PWIDTH, lc->Msg) )
		Write_Log(0,lc->Msg);
	sprintf( lc->Msg, "%d", lc->plate_height );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PHEIGHT, lc->Msg) )
		Write_Log(0,lc->Msg);

	sprintf(log_msg,"[LPR] [Alloc] [[ch%04d]] create lpr e=%d, M=%d, F=%d, excp=%d, m=%d, pw=%d, ph=%d .\n", parking_id, lc->edge, lc->loc_type, lc->format, lc->excp_lv, lc->loc_more, lc->plate_width, lc->plate_height);
	Write_Log(0,log_msg);	
    lc->hImage = OCR_CreateImage(lc->width, lc->width, lc->height, 8, TRUE);
    if(lc->hImage == NULL) {
		//ShowError();
		sprintf(log_msg,"[LPR] [Alloc] [ERROR] [ch%04d] OCR_CreateImage fail.\n", lc->park_id);
		Write_Log(0,log_msg);
        lpr_free(lc);
        return 0;
    }

	//lc->ans_list = (lpr_result*)malloc(sizeof(lpr_result)*lc->loc_more*20);
	lc->ans_list = result;
	if(!lc->ans_list)
	{
		sprintf(log_msg,"[LPR] [Alloc] [ERROR] [ch%04d] ans_list = 0.\n", lc->park_id);
		Write_Log(0,log_msg);
		//ShowError();
		lpr_free(lc);
		return 0;
	}

	//int x;
	//for (x=0 ; x<MAX_CH ; x++)
	//{
	//	tmp_del[x].cout = 0;
	//}
	
	sprintf(log_msg,"[LPR] [Alloc] lpr_alloc() success.\n");
	Write_Log(0,log_msg);
	
    //lc->width = width;
    //lc->height = height;

	return lc;
}

int   lpr_process(void *handle, void *buf, u32 timestamp, u32 micro_sec)
{
	char		log_msg[256];
	char		path[128];
	char		*log;
	int			sret;
    lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;
	int t ;
	if(lc->magic != LPR_MAGIC || !buf)
		return -1;

	struct tm ttb;
	localtime_r(&timestamp, &ttb);
	BYTE *tmp_buf;
	tmp_buf = (BYTE*)malloc(sizeof(BYTE)*76800);
	//if (tmp_buf == NULL)
	//{
	//	Write_Log(0,"malloc fail\n");
	//	return -1;
	//}
	//lc->hImage = OCR_CreateImage(320,320,240,8,0);
	//OCR_CopyImage(lc->hImage,buf);
	if( memcpy(tmp_buf, buf, 76800) == 0){
		sprintf(log_msg,"[LPR] [Process] [ERROR] memcpy fail");
		Write_Log(0,log_msg);}


	sprintf(log_msg,"[LPR] [Process] [ch%04d] hook image. \n", lc->park_id);
	Write_Log(0,log_msg);
	if(!ImageHookBuffer(lc->hImage, (LPSTR)tmp_buf)) {
		sprintf(log_msg,"[LPR] [Process] [ch%04d] ImageHookBuffer ERROR. \n", lc->park_id);
		Write_Log(0,log_msg);
		return -1;
	}

if( dedug_flag)
{
	//lc->hImage = OCR_LoadImage("OBNVR.obi",0,0);
	sprintf(path, "/dbg/Before-%02d.obi", lc->park_id);
	t=OCR_WriteImage(path, lc->hImage, IMG_FMT_OBI, 0);
	sprintf(log_msg,"[LPR] [Process] OCR %d\n",t);
	//memcpy(lc->hImage,buf,320*240);
}

	sprintf(log_msg,"[LPR] [Process] [ch%04d] lpr image(%d)/(%d). \n", lc->park_id, lc, lc->hLpr);
	Write_Log(0,log_msg);
    if(!WNLPRRecog1(lc->hLpr, lc->hImage, &lc->PicRst)) {
        //ShowError();
		sprintf(log_msg,"[LPR] [Process] [ch%04d] WNLPRRecog1 ERROR. \n", lc->park_id);
		Write_Log(0,log_msg);
		ImageUnhookBuffer(lc->hImage);
        return -1;
    }
	sprintf(log_msg,"[LPR] [Process] [ch%04d] lpr finish. \n", lc->park_id);
	Write_Log(0,log_msg);

#if 0
	sprintf(log_msg,"[LPR] [Process] [DBG] [AFT] [ch%04d], width=%d, height=%d ,buf(%X). \n", lc->park_id, lc->width, lc->height, buf);
	Write_Log(0,log_msg);	
	sprintf(path, "/dbg/After%02d.obi", lc->park_id);
	t=OCR_WriteImage(path, lc->hImage, IMG_FMT_OBI, 0);
	sprintf(log_msg,"[LPR] [Process] OCR %d\n",t);
	Write_Log(0,log_msg);
	//RAW2File(path, buf, lc->width, lc->height, 8, IMG_FMT_OBI, 0);
#endif
	//ImageUnhookBuffer(lc->hImage);
	//save lpr image for dbg
#if 0
    {
        FILE *fp = fopen("out.y", "wb");
        if(fp) {
			fwrite((BYTE*)lc->hImage, 1, 20, fp);
            fwrite(buf, 1, 320*240, fp);
            fclose(fp);
        }
    }
#endif

	//show lpr result info
#if 1
	sprintf(log_msg,"[LPR] [Process] [ch%04d] check list\n", lc->park_id);
	Write_Log(0,log_msg);
    int i, j;
    for(i=0; i<lc->PicRst.nPlateNo; i++){
		//ob_info
		sprintf(log_msg,"[LPR] [Process] [ch%04d] \t\t Total_Ans#%d , P#%d: %s@(%d,%d)(%d,%d)\n", lc->park_id,lc->PicRst.nPlateNo, i+1,
                lc->PicRst.PltRst[i].szCarNo,
                lc->PicRst.PltRst[i].rectLoc.left, lc->PicRst.PltRst[i].rectLoc.top,
                lc->PicRst.PltRst[i].rectLoc.right, lc->PicRst.PltRst[i].rectLoc.bottom );
	Write_Log(0,log_msg);}

	char	tmp_path[128];
	if(lc->PicRst.nPlateNo == 0)
	{//##-####
		sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld>  no ans in this frame.\n", lc->park_id, lc->ans_list);
		Write_Log(0,log_msg);

		for(j=0 ; j<MAX_CH ; j++)
		{
			if (lc->ans_list[j].cout > 0 )
			{
				if( strcmp(lc->ans_list[j].plate ,"##-####") == 0)
				{
					sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld> plate %s - find in list[%d], cout=%d.\n", lc->park_id, lc->ans_list, lc->PicRst.PltRst[i].szCarNo, j, lc->ans_list[j].cout);
					Write_Log(0,log_msg);
					break;
				}
			}
			else if( lc->ans_list[j].cout == 0 && j==0){
			//if(strcmp("0000" ,lc->ans_list[j].plate) == 0 )
			//{
				struct tm tt;
				char str[32];
				localtime_r(&timestamp, &tt);
				sprintf(str, "%04d%02d%02d%02d/%04d_%02d%02d%03d",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, lc->park_id, tt.tm_min, tt.tm_sec, micro_sec/1000);

				memset(tmp_path, 0, 128);
				//sprintf(tmp_path, "/dbg/lpr/%04d_%s_##-####.obi", lc->park_id, str);
				sprintf(tmp_path, "/dbg/lpr/%s_##-####.obi", str);
				//sprintf(lc->ans_list[j].image_path, "/dbg/lpr/%04d_%s_##-####.obi", lc->park_id, str);
				//sprintf(lc->ans_list[j].image_path, "/dbg/%04d_%s_##-####.obi", lc->park_id, str);
				sprintf(log_msg,"[LPR] [Process] [ch%04d] plate ##-#### write obi - %s.\n", lc->park_id, tmp_path);
				Write_Log(0,log_msg);

 				FILE *fp = fopen(tmp_path, "wb");
 				if(!fp) { //will - need check write obi fail 
  					sprintf(log_msg,"[LPR] [Process] [ERROR] [ch%04d] plate ##-#### write obi fail- %s.\n", lc->park_id,  tmp_path);
  					Write_Log(0,log_msg);
					break;
  				}
  				else
 				{//success
 					fwrite((BYTE*)lc->hImage, 1, 20, fp);
 					fwrite(buf, 1, 320*240, fp);
 					fclose(fp);
 					
					sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld> plate ##-#### - write obi success - %s.\n", lc->park_id, lc->ans_list,  tmp_path);
					Write_Log(0,log_msg);
					strcpy(lc->ans_list[j].image_path, tmp_path);

					sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld>  plate ##-#### - strcpy end - %s.\n", lc->park_id, lc->ans_list,  lc->ans_list[j].image_path);
					Write_Log(0,log_msg);
					strcpy(lc->ans_list[j].plate, "##-####");

					lc->ans_list[j].x1 = 0;
					lc->ans_list[j].y1 = 0;
					lc->ans_list[j].x2 = 0;
					lc->ans_list[j].y2 = 0;

					lc->ans_list[j].timestamp = timestamp;
					lc->ans_list[j].micro_sec = micro_sec;
					lc->ans_list[j].cout++;

					sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld> plate ##-#### - add in list[%d], cout=%d.\n", lc->park_id, lc->ans_list, j, lc->ans_list[j].cout);
					Write_Log(0,log_msg);
				}
				if((sret=chmod(tmp_path,S_ISGID|S_IWUSR|S_IXUSR|S_IRGRP|S_IROTH))<0)
				{
				sprintf(log_msg,"[LPR] [Process] [FILE] chmod fail\n");
				Write_Log(0,log_msg);
				}
				break;
			}		
		}
	}
	else
	{
		for(i=0; i<lc->PicRst.nPlateNo; i++)
		{
			for(j=0 ; j<MAX_CH ; j++)
			{
				if (lc->ans_list[j].cout > 0)
				{
					sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld> plate %s - search list[%d]-%s(%d).\n", lc->park_id, lc->ans_list, lc->PicRst.PltRst[i].szCarNo, j, lc->ans_list[j].plate, lc->ans_list[j].cout);
					Write_Log(0,log_msg);

					if( strcmp(lc->PicRst.PltRst[i].szCarNo ,lc->ans_list[j].plate) == 0)
					{
						lc->ans_list[j].cout++;
						sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld> plate %s - find in list[%d], cout=%d.\n", lc->park_id, lc->ans_list, lc->PicRst.PltRst[i].szCarNo, j, lc->ans_list[j].cout);
						Write_Log(0,log_msg);
						break;
					}
				}
				
				if (lc->ans_list[j].cout == 0)
				{
					struct tm tt;
					char str[32];
					localtime_r(&timestamp, &tt);
					sprintf(str, "%04d%02d%02d%02d/%04d_%02d%02d%03d_%s",
						tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, lc->park_id, tt.tm_min, tt.tm_sec, micro_sec/1000, lc->PicRst.PltRst[i].szCarNo);

					memset(tmp_path, 0, 128);
					//sprintf(tmp_path, "/dbg/lpr/%04d_%s_%s.obi", lc->park_id, str, lc->PicRst.PltRst[i].szCarNo);
					sprintf(tmp_path, "/dbg/lpr/%s.obi", str);
					sprintf(log_msg,"[LPR] [Process] [ch%04d] plate %s write obi - %s.\n", lc->park_id, lc->PicRst.PltRst[i].szCarNo, tmp_path);
					Write_Log(0,log_msg);

 					FILE *fp = fopen(tmp_path, "wb");
 						if(!fp) {
 						
  //will - need check write obi fail 
  						sprintf(log_msg,"[LPR] [Process] [ch%04d] [ERROR] plate %s write obi fail- %s.\n", lc->park_id, lc->PicRst.PltRst[i].szCarNo, tmp_path);
  						Write_Log(0,log_msg);
						break;
  					}
  					else
 					{//success
 						fwrite((BYTE*)lc->hImage, 1, 20, fp);
 						fwrite(buf, 1, 320*240, fp);
 						fclose(fp);

						sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld> plate %s - write obi success - %s.\n", lc->park_id, lc->ans_list, lc->ans_list[j].plate,  tmp_path);
						Write_Log(0,log_msg);
						strcpy(lc->ans_list[j].image_path, tmp_path);

						sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld>  plate %s - strcpy - %s.\n", lc->park_id, lc->ans_list, lc->ans_list[j].plate, lc->ans_list[j].image_path);
						Write_Log(0,log_msg);
						strcpy(lc->ans_list[j].plate, lc->PicRst.PltRst[i].szCarNo);

						lc->ans_list[j].x1 = lc->PicRst.PltRst[i].rectLoc.top;
						lc->ans_list[j].y1 = lc->PicRst.PltRst[i].rectLoc.left;
						lc->ans_list[j].x2 = lc->PicRst.PltRst[i].rectLoc.bottom;
						lc->ans_list[j].y2 = lc->PicRst.PltRst[i].rectLoc.right;

						lc->ans_list[j].timestamp = timestamp;
						lc->ans_list[j].micro_sec = micro_sec;

						//memcpy(lc->ans_list[j].imgbuf, buf, 320*240);
						lc->ans_list[j].cout++;

						sprintf(log_msg,"[LPR] [Process] [ch%04d] <%ld>  plate %s - add in list[%d], cout=%d.\n", lc->park_id, lc->ans_list, lc->PicRst.PltRst[i].szCarNo, j, lc->ans_list[j].cout);
						Write_Log(0,log_msg);

					}
					if((sret=chmod(tmp_path,S_ISGID|S_IWUSR|S_IXUSR|S_IRGRP|S_IROTH))<0)
					{
						sprintf(log_msg,"[LPR] [Process] chmod fail\n");
						Write_Log(0,log_msg);
					}
					break;
				}	
			}

			if(j == MAX_CH)
			{//答案超過list總數
				sprintf(log_msg,"[LPR] [Process] [ch%04d] [WARNING]list full, plate=%s can not insert. \n", lc->park_id, lc->PicRst.PltRst[i].szCarNo);
				lpr_list_clean(lc, lc->park_id, 1);
				Write_Log(0,log_msg);
			}
		}
	}

#endif

	if(!ImageUnhookBuffer(lc->hImage))
		Write_Log(0,"ImageUnhookBuffer == 0\n");
	free(tmp_buf);

    return 0;
}

int   lpr_dump_result(void *handle, void* priv, void *img_buf)
{
	char				log_msg[256];
    lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;
	result_info *result = (result_info*) priv;

    if(lc->magic != LPR_MAGIC)
        return -1;

    //ob_info("# of results: %d\n", lc->PicRst.nPlateNo);

    int i;
    /* for(i=0; i<lc->PicRst.nPlateNo; i++){	 
         sprintf(log_msg,"[RESULT c%d] P#%d: %s@(%d,%d)(%d,%d)\n", lc->park_id, i+1,
                lc->PicRst.PltRst[i].szCarNo,
                 lc->PicRst.PltRst[i].rectLoc.left, lc->PicRst.PltRst[i].rectLoc.top,
				 lc->PicRst.PltRst[i].rectLoc.right, lc->PicRst.PltRst[i].rectLoc.bottom );
		 Write_Log(0,log_msg);
	 }*/
	sprintf(log_msg,"[LPR] [DMP] [ch%04d] <%ld>  get into dump function. \n", lc->park_id, lc->ans_list);
	Write_Log(0,log_msg);

 	int max, list_num;
	max = list_num = 0;
	for (i=0 ; i< MAX_CH ; i++)
	{
		sprintf(log_msg,"[LPR] [DMP] [ch%04d] <%ld> ans - %s ,cout=%d . \n", lc->park_id, lc->ans_list, lc->ans_list[i].plate, lc->ans_list[i].cout);
		Write_Log(0,log_msg);
		if(lc->ans_list[i].cout == 0)
			break;
		else if(max < lc->ans_list[i].cout)
		{
			max = lc->ans_list[i].cout;
			list_num = i;
		}
		else if (max == lc->ans_list[i].cout && lc->ans_list[i].cout == 1)
		{
			list_num = 1;
		}
		
	}

	if(max == 0)
		strcpy(result->plate, "##-####");
	else
	{	
		strcpy(result->plate, lc->ans_list[list_num].plate );
		
		result->x1 = lc->ans_list[list_num].x1;
		result->y1 = lc->ans_list[list_num].y1;
		result->x2 = lc->ans_list[list_num].x2;
		result->y2 = lc->ans_list[list_num].y2;
		
		//memcpy((LPSTR)result->imgbuf, lc->ans_list[list_num].imgbuf, 320*240);
	}

	sprintf(log_msg,"[LPR] [DMP] [ch%04d] <%ld> ans - %s ,cout=%d ,is the max cout ,list_num = %d. \n", lc->park_id, lc->ans_list, lc->ans_list[list_num].plate, max, list_num);
	Write_Log(0,log_msg);
	//memcpy((LPSTR)img_buf, lc->ans_list[list_num].imgbuf, 320*240);
	
    //ImageUnhookBuffer(lc->hImage);

    return max;
}

int lpr_list_clean(void *handle, int ch_id, int max)
{
	char				log_msg[256];
	char				temp_plate[64];
	int					flag = 0;
	int					temp=0;
	lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;

	int		i;
	int		temp_count = 5;

	for(i=0 ; i<MAX_CH ; i++)
	{
		if(lc->ans_list[i].cout > 0)
		{
			sprintf(log_msg,"[LPR] [CLR] [ch%04d] <%ld> - list[%d]-%s(%d). \n", lc->park_id, lc->ans_list, i, lc->ans_list[i].plate, lc->ans_list[i].cout);
			Write_Log(0,log_msg);
		}

		if(max == 1)
		{
			sprintf(log_msg,"[LPR] [CLR] [MAX] [ch%04d] - list[%d]-%s - cout=%d. \n", lc->park_id, i, lc->ans_list[i].plate, lc->ans_list[i].cout);
			Write_Log(0,log_msg);
			if( lc->ans_list[i].cout >= temp_count)
			{
				sprintf(log_msg,"[LPR] [CLR] [MAX] [ch%04d] - Car_No = %s - cout=%d. \n", lc->park_id, lc->ans_list[i].plate, lc->ans_list[i].cout);
				Write_Log(0,log_msg);
				temp_count = lc->ans_list[i].cout;
				//memset(lc->ans_list[i].plate ,0 ,32);
				if(i != 0)
				{
					strcpy(lc->ans_list[0].plate, lc->ans_list[i].plate);
					sprintf(log_msg,"[LPR] [CLR] [MAX] i!=0 <%ld> clean [ch%04d] - list[%d]-%s - cout=%d. list[0] = %s\n", lc->ans_list, lc->park_id, i, lc->ans_list[i].plate, lc->ans_list[i].cout,lc->ans_list[0].plate);
					Write_Log(0,log_msg);
					sprintf(lc->ans_list[i].plate, "0000");
					lc->ans_list[0].cout = lc->base_number;
					lc->ans_list[i].cout = 0;
				}
				else 
				{
					lc->ans_list[0].cout = lc->base_number;				
				}
				sprintf(log_msg,"[LPR] [CLR] [MAX] <%ld> clean [ch%04d] - list[%d]-%s - cout=%d. list[0] = %s - cout=%d ,\n", lc->ans_list, lc->park_id, i, lc->ans_list[i].plate, lc->ans_list[i].cout,lc->ans_list[0].plate,lc->ans_list[0].cout);
				Write_Log(0,log_msg);
				//memset(lc->ans_list[i].plate ,0 ,32);
				flag = 1;
			}	
			else
			{
				strcpy(temp_plate,lc->ans_list[i].plate);
				memset(lc->ans_list[i].plate ,0 ,32);
				sprintf(lc->ans_list[i].plate, "0000");

				//strcpy(tmp_del[i].image_path, lc->ans_list[i].image_path);
				//tmp_del[i].cout = lc->ans_list[i].cout;
				lc->ans_list[i].cout = 0;

				sprintf(log_msg,"[LPR] [CLR] [NOT MAX] <%ld> clean [ch%04d] - list[%d]-%s - cout=%d. \n", lc->ans_list, lc->park_id, i, lc->ans_list[i].plate, lc->ans_list[i].cout);
				Write_Log(0,log_msg);			
			}
		}
		else 
		{
			if( lc->ans_list[i].cout > 0)
			{
				memset(lc->ans_list[i].plate ,0 ,32);
				sprintf(lc->ans_list[i].plate, "0000");

				//strcpy(tmp_del[i].image_path, lc->ans_list[i].image_path);
				//tmp_del[i].cout = lc->ans_list[i].cout;
				lc->ans_list[i].cout = 0;

				sprintf(log_msg,"[LPR] [CLR] <%ld> clean [ch%04d] - list[%d]-%s - cout=%d. \n", lc->ans_list, lc->park_id, i, lc->ans_list[i].plate, lc->ans_list[i].cout);
				Write_Log(0,log_msg);
				flag = 1;
			}	
		}
		/*if( tmp_del[i].cout > 0)
		{
			if( remove(tmp_del[i].image_path) < 0)
			{
				sprintf(log_msg,"[LPR] [CLR] <%ld> ch%d - del file %s fail. \n", lc->ans_list, lc->park_id, lc->ans_list[i].image_path);
				Write_Log(0,log_msg);
				//sprintf(sys_cmd, "rm -rf //dbg//%04d_*.obi" );
				//system(sys_cmd);
			}

			sprintf(log_msg,"[LPR] [CLR] <%ld> ch%d - del file %s success. \n", lc->ans_list, lc->park_id, lc->ans_list[i].image_path);
			Write_Log(0,log_msg);
			tmp_del[i].cout = 0;
		}*/
	}

	if (flag == 0 && max == 1)
	{
		strcpy(lc->ans_list[0].plate,temp_plate);
		sprintf(log_msg,"[LPR] [CLR] [ch%04d] <%ld> - flag==0 temp_plate = %s lc->ans_list[0].plate = %s. \n", lc->park_id, lc->ans_list, temp_plate, lc->ans_list[0].plate);
		Write_Log(0,log_msg);
		lc->ans_list[0].cout = lc->base_number;
	}
	

	return 0;
}

//int   lpr_list_sort(int ch_id, char *Retuen_Ans)
//{
//	lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;
//	result_info			tmp_compare[MAX_CH];
//	char				log_msg[256];
//	char				temp_plate[16];
//	int tmp_cout[MAX_CH]; 
//	int cout = 0;
//	int x,y,flag;
//
//	memset(tmp_cout, 0, MAX_CH);
//	strcpy(Retuen_Ans,"|");
//	for (x=0 ; x<MAX_CH ; x++)
//	{
//		if (tmp_del[x].cout == 0){break;}
//		cout++;
//	}
//
//	sprintf(log_msg,"[LPR] [SORT] [ch%04d], list cout=%d. \n", parking_id, cout);
//	Write_Log(0,log_msg);
//
//	int y;
//	int base;
//	for (x=0 ; x<cout ; x++)
//	{		
//		for (y=0 ; y<cout ; y++)
//		{
//
//		}
//		
//		
//		
//		sprintf(log_msg,"[LPR] [SORT] [ch%04d] tmp_list[%d] = %s(%d) . \n", parking_id, x, result_list[sort_list[x]].plate, result_list[sort_list[x]].cout);
//		Write_Log(0,log_msg);
//		tmp_cout[sort_list[x]] = 0;
//	}
//	
//
//	return 0;
//}