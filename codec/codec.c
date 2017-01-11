/*!
 *  @file       codec.c
 *  @version    1.0
 *  @date       05/05/2014
 *  @author     Jacky Hsu <Jacky_Hsu@orbit.com.tw>
 *  @note       Codec APIs
 */

// ----------------------------------------------------------------------------
// Include Files
// ----------------------------------------------------------------------------
#include "ob_platform.h"
#include "dbg_msg/dbg_log.h"
#include "OBImgPrc.h"

// codec headers
#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vda.h"
#include "hi_comm_region.h"
#include "hi_comm_hdmi.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_vda.h"
#include "mpi_region.h"
#include "mpi_hdmi.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_aenc.h"
#include "mpi_adec.h"
#include "codec.h"
// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "CODEC"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------
#define CH_NR   4
typedef struct  
{
	LPBYTE    lpByte;
	LPBYTE    lpByte2;
}deter_byte;
//#define NR_SD       1
//#define NR_960H     0
//#define NR_FHD      1

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------
static int c_index = 0;
static pthread_mutex_t i_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t c_mutex[CH_NR];
deter_byte	d_byte[1];

/////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI JRDetectionEdge( LPBYTE lpIn, LPBYTE lpOut, int nWidth, int nHeight, LPRECT lpBound, int vvv)
{                       
	int  i, j;
	WORD  K2,K1,K3,K4;

	for( i=lpBound->top+1; i<=lpBound->bottom-1; i++ )
		for( j=lpBound->left+1; j<=lpBound->right-1; j++ )
		{
			if(vvv==1)
			{
				K1 = abs( lpIn[nWidth*(i-1)+j-1] + lpIn[nWidth*(i-1)+j]*2 + lpIn[nWidth*(i-1)+j+1]             //ㆄ��
				- lpIn[nWidth*(i+1)+j-1] - lpIn[nWidth*(i+1)+j]*2 - lpIn[nWidth*(i+1)+j+1] );
				lpOut[nWidth*i+j] = min( 255, K1);
			}
			else if(vvv==2)
			{
				K1 = 0 ;
				K2 = abs( lpIn[nWidth*(i-1)+j+1] + lpIn[nWidth*i+j+1]*2   + lpIn[nWidth*(i+1)+j+1]                //カそ
				- lpIn[nWidth*(i-1)+j-1] - lpIn[nWidth*i+j-1]*2   - lpIn[nWidth*(i+1)+j-1] );
				lpOut[nWidth*i+j] = min( 255, K2);
			}
			else if(vvv==3)
			{
				K1 = abs( lpIn[nWidth*i+j+1] + lpIn[nWidth*(i+1)+j+1]*2  + lpIn[nWidth*(i+1)+j]                      //쩵그45プ
				-lpIn[nWidth*i+(j-1)] - lpIn[nWidth*(i-1)+(j-1)]*2  - lpIn[nWidth*(i-1)+j] );
				K2 = abs( lpIn[nWidth*i+(j-1)] + lpIn[nWidth*(i+1)+(j-1)]*2  + lpIn[nWidth*(i+1)+j]          //ⅹ그135プ
				-lpIn[nWidth*(i-1)+j] - lpIn[nWidth*(i-1)+(j+1)]*2  - lpIn[nWidth*i+(j-1)] );
				lpOut[nWidth*i+j] = min( 255, K2);
			}
			else if(vvv==6)
			{
				K1 = abs( lpIn[nWidth*(i-1)+j-1] + lpIn[nWidth*(i-1)+j]*2 + lpIn[nWidth*(i-1)+j+1]             //ㆄ��
				- lpIn[nWidth*(i+1)+j-1] - lpIn[nWidth*(i+1)+j]*2 - lpIn[nWidth*(i+1)+j+1] );
				K2 = abs( lpIn[nWidth*(i-1)+j+1] + lpIn[nWidth*i+j+1]*2   + lpIn[nWidth*(i+1)+j+1]                //カそ
				- lpIn[nWidth*(i-1)+j-1] - lpIn[nWidth*i+j-1]*2   - lpIn[nWidth*(i+1)+j-1] );
				K3 = abs( lpIn[nWidth*(i-1)+j] + lpIn[nWidth*(i-1)+j+1]*2  + lpIn[nWidth*i+j+1]                       //쩵그
				-lpIn[nWidth*i+(j-1)] - lpIn[nWidth*(i+1)+(j-1)]*2  - lpIn[nWidth*(i+1)+j] );
				K4 = abs( lpIn[nWidth*i+(j-1)] + lpIn[nWidth*(i-1)+(j-1)]*2  + lpIn[nWidth*(i-1)+j]            //ⅹ그
				-lpIn[nWidth*i+(j+1)] - lpIn[nWidth*(i+1)+(j+1)]*2  - lpIn[nWidth*(i+1)+j] );
				lpOut[nWidth*i+j] = min( 255, (K1 + K2 + K3 + K4) / 4);
			}
			else 
			{
				K1 = abs( lpIn[nWidth*(i-1)+j-1] + lpIn[nWidth*(i-1)+j]*2 + lpIn[nWidth*(i-1)+j+1]
				- lpIn[nWidth*(i+1)+j-1] - lpIn[nWidth*(i+1)+j]*2 - lpIn[nWidth*(i+1)+j+1] );
				K2 = abs( lpIn[nWidth*(i-1)+j+1] + lpIn[nWidth*i+j+1]*2   + lpIn[nWidth*(i+1)+j+1]
				- lpIn[nWidth*(i-1)+j-1] - lpIn[nWidth*i+j-1]*2   - lpIn[nWidth*(i+1)+j-1] );
				lpOut[nWidth*i+j] = min( 255, (K1 + K2)/2);
			}                               
			//             k3 = abs( lpIn[nWidth*(i-1)+j] + lpIn[nWidth*(i-1)+j+1]  + lpIn[nWidth*i+j+1]
			//                     -lpIn[nWidth*i+(j-1)] - lpIn[nWidth*(i+1)+(j-1)]  - lpIn[nWidth*(i+1)+j] );

		}
		return( TRUE );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////



// ----------------------------------------------------------------------------
// _GrayMotionRate Functions
// ----------------------------------------------------------------------------
static int _GrayMotionRate( HANDLE hGray1, int nWidth1, HANDLE hGray2, int nWidth2, int nHeight, LPRECT lpRect)
{
	int      i, j, h, w, nDiff, nStep1, nStep2;
	LPBYTE   lpImg1, lpImg2, p1, p2;
	int      Img1, Img2;
	RECT     Rect;
	static char				log_msg[256];

	memcpy( &Rect, lpRect, sizeof(RECT) );

	h = Rect.bottom - Rect.top + 1;
	w = Rect.right  - Rect.left+ 1;
	lpImg1 = (LPBYTE)hGray1 + Rect.top*nWidth1 + Rect.left;
	lpImg2 = (LPBYTE)hGray2 + Rect.top*nWidth2 + Rect.left;
	nDiff = 0;
	nStep1 = nWidth1*2;
	nStep2 = nWidth2*2;
	for( i=0; i<h-2; i+=2, lpImg1+=nStep1, lpImg2+=nStep2 )
		for( j=0, p1=lpImg1, p2=lpImg2; j<w-2; j+=2, p1+=2, p2+=2 )
		{
			Img1 =   ( *(p1)+ *(p1+1)+ *(p1+2) 
				+ *(p1+nWidth1)+ *(p1+nWidth1+1)+ *(p1+nWidth1+2) 
				+ *(p1+nWidth1*2) + *(p1+nWidth1*2+1)+ *(p1+nWidth1*2+2) );
			Img2 =   ( *(p2)+ *(p2+1)+ *(p2+2) 
				+ *(p2+nWidth2)+ *(p2+nWidth2+1)+ *(p2+nWidth2+2) 
				+ *(p2+nWidth2*2) + *(p2+nWidth2*2+1)+ *(p2+nWidth2*2+2) );
			nDiff += abs(Img1-Img2);
		}
		nDiff /= h*w*9/4;

		return( nDiff );
}
// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
#if 0
static int codec_sys_init(void)
{
	char				log_msg[256];
	VB_CONF_S stVbConf;
    MPP_SYS_CONF_S stSysConf = {0};
    HI_S32 s32Ret = HI_FAILURE;

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    memset(&stVbConf,0,sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt = 128;
    s32Ret = HI_MPI_VB_SetConf(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        //ob_error("HI_MPI_VB_SetConf failed!\n");
		sprintf(log_msg,"[CODEC] HI_MPI_VB_SetConf failed!\n");
		Write_Log(0,log_msg);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != s32Ret)
    {
        //ob_error("HI_MPI_VB_Init failed!\n");
		sprintf(log_msg,"[CODEC] HI_MPI_VB_Init failed!\n");
		Write_Log(0,log_msg);
        return HI_FAILURE;
    }

    stSysConf.u32AlignWidth = 64;
    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if (HI_SUCCESS != s32Ret)
    {
        //ob_error("HI_MPI_SYS_SetConf failed\n");
		sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetConf failed\n");
		Write_Log(0,log_msg);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32Ret)
    {
        //ob_error("HI_MPI_SYS_Init failed!\n");
		sprintf(log_msg,"[CODEC] HI_MPI_SYS_Init failed!\n");
		Write_Log(0,log_msg);
        return HI_FAILURE;
    }

    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnVDEC;

    /* VDEC chn max is 32*/
    for(i=0;i<32;i++)
    {
        stMppChnVDEC.enModId = HI_ID_VDEC;
        stMppChnVDEC.s32DevId = 0;
        stMppChnVDEC.s32ChnId = i;

        if(0 == (i%2))
        {
            pcMmzName = NULL;
        }
        else
        {
            pcMmzName = "ddr1";
        }

        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnVDEC,pcMmzName);
        if (s32Ret)
        {
            //ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}
#else
static int codec_sys_init(void)
{
	char				log_msg[256];
	int ret;
	int i = 0;
	int z;
	VB_CONF_S vb_conf = {0};
	MPP_SYS_CONF_S sys_conf = {0};
	u32 reg;

	HI_MPI_SYS_GetReg(0x205ca000, &reg);
	HI_MPI_SYS_SetReg(0x205ca000, (reg&0x7fffffff));

	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();

	memset(&vb_conf, 0, sizeof(vb_conf));

	i = 0;
	vb_conf.u32MaxPoolCnt = 128;

#if 1 // only FHD
    // FHD
	//for (z=0;z<25;z++)
	//{
	//	d_byte[z].lpByte = malloc(1000000);
	//	d_byte[z].lpByte2 = malloc(1000000);
	//	memset(d_byte[z].lpByte,0,1000000);
	//	memset(d_byte[z].lpByte2,0,1000000);
	//}	

	Write_Log(0,"[CODEC] INIT \n");
    vb_conf.astCommPool[i].u32BlkSize = (1920 * 1088 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = 32;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[0].acMmzName));
    i++;

    vb_conf.astCommPool[i].u32BlkSize = (1920 * 1088 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = 32;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = 32;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[1].acMmzName));
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = 32;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;
#else
#if NR_SD > 0
    // D1
    vb_conf.astCommPool[i].u32BlkSize = (768 * 480 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = 5 * NR_SD;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[0].acMmzName));
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = 5 * NR_SD;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[1].acMmzName));
    i++;
#endif

#if NR_960H > 0
    vb_conf.astCommPool[i].u32BlkSize = (960 * 480 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = 5 * NR_960H;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[0].acMmzName));
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = 5 * NR_960H;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[1].acMmzName));
    i++;
#endif

#if NR_FHD > 0
    // FHD
    vb_conf.astCommPool[i].u32BlkSize = (1920 * 1088 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = NR_FHD * 5;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[0].acMmzName));
    i++;

    vb_conf.astCommPool[i].u32BlkSize = (1920 * 1088 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = NR_FHD * 5;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;

    vb_conf.astCommPool[i].u32BlkSize = (960 * 480 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = NR_FHD * 3;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[0].acMmzName));
    i++;

    vb_conf.astCommPool[i].u32BlkSize = (960 * 480 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = NR_FHD * 3;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = NR_FHD * 2;
    memset(vb_conf.astCommPool[i].acMmzName, 0, sizeof(vb_conf.astCommPool[1].acMmzName));
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = NR_FHD * 2;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;
#endif


#if NR_SD > 0
    vb_conf.astCommPool[i].u32BlkSize = (768 * 480 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = NR_SD * 5;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = NR_SD * 5;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;
#endif

#if NR_960H > 0
    vb_conf.astCommPool[i].u32BlkSize = (960 * 480 * 3) >> 1;
    vb_conf.astCommPool[i].u32BlkCnt = NR_960H * 5;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;

    /*hist buf*/
    vb_conf.astCommPool[i].u32BlkSize = (196 * 4);
    vb_conf.astCommPool[i].u32BlkCnt = NR_960H * 5;
    strcpy(vb_conf.astCommPool[i].acMmzName, "ddr1");
    i++;
#endif
#endif

	ret = HI_MPI_VB_SetConf(&vb_conf);
	if (ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VB_SetConf failed!\n");
		sprintf(log_msg,"[CODEC] HI_MPI_VB_SetConf failed!\n");
		Write_Log(0,log_msg);
		return -1;
	}

	ret = HI_MPI_VB_Init();
	if (ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VB_Init failed!\n");
		sprintf(log_msg,"[CODEC] HI_MPI_VB_Init failed!\n");
		Write_Log(0,log_msg);
		return -1;
	}

	sys_conf.u32AlignWidth = 64;
	ret = HI_MPI_SYS_SetConf(&sys_conf);
	if (ret != HI_SUCCESS) {
		//ob_error("HI_MPI_SYS_SetConf failed\n");
		sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetConf failed\n");
		Write_Log(0,log_msg);
		return -1;
	}

	ret = HI_MPI_SYS_Init();
	if (ret != HI_SUCCESS) {
		//ob_error("HI_MPI_SYS_Init failed!\n");
		sprintf(log_msg,"[CODEC] HI_MPI_SYS_Init failed!\n");
		Write_Log(0,log_msg);
		return -1;
	}

	char *mmz_name = NULL;
	MPP_CHN_S mpp_ch_vi;
	MPP_CHN_S mpp_ch_vo;
	MPP_CHN_S mpp_ch_vpss;
	MPP_CHN_S mpp_chn_grp;
	MPP_CHN_S mpp_ch_venc;
	MPP_CHN_S mpp_ch_rgn;
	MPP_CHN_S mpp_ch_vdec;
	MPP_CHN_S mpp_ch_vda;

	for (i=0; i<32; i++) {
		mpp_ch_vi.enModId = HI_ID_VIU;
		mpp_ch_vi.s32DevId = 0;
		mpp_ch_vi.s32ChnId = i;

		mpp_ch_vdec.enModId = HI_ID_VDEC;
		mpp_ch_vdec.s32DevId = 0;
		mpp_ch_vdec.s32ChnId = i;

		mpp_ch_vda.enModId = HI_ID_VDA;
		mpp_ch_vda.s32DevId = 0;
		mpp_ch_vda.s32ChnId = i;

        if ((i%2) == 0)
            mmz_name = NULL;
        else
            mmz_name = "ddr1";

		/*vi*/
		ret = HI_MPI_SYS_SetMemConf(&mpp_ch_vi, mmz_name);
		if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
			return -1;
		}

		/*vdec*/
		ret = HI_MPI_SYS_SetMemConf(&mpp_ch_vdec, mmz_name);
		if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR @ %d, %x!\n", i, ret);
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR @ %d, %x!\n", i, ret);
			Write_Log(0,log_msg);
			return -1;
		}

		/*vda*/
		ret = HI_MPI_SYS_SetMemConf(&mpp_ch_vda, mmz_name);
		if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
			return -1;
		}
	}

	for (i=0; i<64; i++) {
		mpp_ch_vpss.enModId  = HI_ID_VPSS;
		mpp_ch_vpss.s32DevId = i;
		mpp_ch_vpss.s32ChnId = 0;

		mpp_chn_grp.enModId  = HI_ID_GROUP;
		mpp_chn_grp.s32DevId = i;
		mpp_chn_grp.s32ChnId = 0;

		mpp_ch_venc.enModId = HI_ID_VENC;
		mpp_ch_venc.s32DevId = 0;
		mpp_ch_venc.s32ChnId = i;

		if (0 == (i%2))
			mmz_name = NULL;
		else
			mmz_name = "ddr1";

		/*vpss*/
		ret = HI_MPI_SYS_SetMemConf(&mpp_ch_vpss, mmz_name);
		if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
			return -1;
		}

		/*grp*/
		ret = HI_MPI_SYS_SetMemConf(&mpp_chn_grp, mmz_name);
		if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
			return -1;
		}

		/*venc*/
		ret = HI_MPI_SYS_SetMemConf(&mpp_ch_venc, mmz_name);
		if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
			return -1;
		}
	}

	mpp_ch_rgn.enModId  = HI_ID_RGN;
	mpp_ch_rgn.s32DevId = 0;
	mpp_ch_rgn.s32ChnId = 0;

	ret = HI_MPI_SYS_SetMemConf(&mpp_ch_rgn, "ddr1");
	if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
		return -1;
	}

	for (i=0; i<8; i++) {
		mpp_ch_vo.enModId  = HI_ID_VOU;
		mpp_ch_vo.s32DevId = i;
		mpp_ch_vo.s32ChnId = 0;
		ret = HI_MPI_SYS_SetMemConf(&mpp_ch_vo, "ddr1");
		if (ret) {
			//ob_error("HI_MPI_SYS_SetMemConf ERR !\n");
			sprintf(log_msg,"[CODEC] HI_MPI_SYS_SetMemConf ERR !\n");
			Write_Log(0,log_msg);
			return -1;
		}
	}

	return 0;
}
#endif

static int create_jpeg_encoder(int ch, u32 x, u32 y, u32 width, u32 height)
{
    //
    // Create VPSS
    //
	char				log_msg[256];
    VPSS_GRP_ATTR_S stGrpVpssAttr;
    VPSS_CROP_INFO_S stCropInfo;
    HI_S32 s32Ret = HI_SUCCESS;
    VPSS_GRP VpssGrp = ch;
    VPSS_CHN VpssChn = ch;
	sprintf(log_msg,"[CODEC] width = %d , height = %d\n",width,height);
	Write_Log(0,log_msg);
    stGrpVpssAttr.u32MaxW = 1920;
    stGrpVpssAttr.u32MaxH = 1088;
    stGrpVpssAttr.bDrEn = HI_FALSE;
    stGrpVpssAttr.bDbEn = HI_FALSE;
    stGrpVpssAttr.bIeEn = HI_FALSE;
    stGrpVpssAttr.bNrEn = HI_FALSE;
    stGrpVpssAttr.bHistEn = HI_FALSE;
    stGrpVpssAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stGrpVpssAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpVpssAttr);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_CreateGrp error=%x\n", s32Ret);
			sprintf(log_msg,"[CODEC] HI_MPI_VPSS_CreateGrp error=%x\n", s32Ret);
			Write_Log(0,log_msg);
        return -1;
    }

    s32Ret = HI_MPI_VPSS_GetGrpAttr(VpssGrp, &stGrpVpssAttr);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_GetGrpAttr error=%x\n", s32Ret);
			sprintf(log_msg,"[CODEC] HI_MPI_VPSS_GetGrpAttr error=%x\n", s32Ret);
			Write_Log(0,log_msg);
        return -1;
    }

#if 0
    stGrpVpssAttr.bIeEn = HI_TRUE;
    stGrpVpssAttr.bNrEn = HI_TRUE;
    s32Ret = HI_MPI_VPSS_SetGrpAttr(VpssGrp, &stGrpVpssAttr);
    if(s32Ret != HI_SUCCESS)
        return s32Ret;
#endif

    s32Ret = HI_MPI_VPSS_GetCropCfg(VpssGrp, &stCropInfo);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_GetCropCfg error=%x\n", s32Ret);
			sprintf(log_msg,"[CODEC] HI_MPI_VPSS_GetCropCfg error=%x\n", s32Ret);
			Write_Log(0,log_msg);
        return -1;
    }

    stCropInfo.bEnable = 1;
    stCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
    stCropInfo.stCropRect.s32X = x;
    stCropInfo.stCropRect.s32Y = y;
    stCropInfo.stCropRect.u32Width = width;
    stCropInfo.stCropRect.u32Height = height;
    s32Ret = HI_MPI_VPSS_SetCropCfg(VpssGrp, &stCropInfo);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_SetCropCfg error=%x\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VPSS_SetCropCfg error=%x\n", s32Ret);
		Write_Log(0,log_msg);
        return -1;
    }

#if 0
    VPSS_CHN_ATTR_S stChnAttr;
    s32Ret = HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn,&stChnAttr);
    if(s32Ret != HI_SUCCESS)
        return s32Ret;
    stChnAttr.bFrameEn = 0;
    stChnAttr.bSpEn = 1;
    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn,&stChnAttr);
    if(s32Ret != HI_SUCCESS)
        return s32Ret;
#endif

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_EnableChn error=%x\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VPSS_EnableChn error=%x\n", s32Ret);
		Write_Log(0,log_msg);
        return -1;
    }
    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_StartGrp error=%x\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VPSS_StartGrp error=%x\n", s32Ret);
		Write_Log(0,log_msg);
        return -1;
    }

    //
    // Create VENC
    //
    VENC_GRP VencGrp = ch;
    VENC_CHN VencChn = ch;
    VENC_CHN_ATTR_S stVencChnAttr;
	VENC_ATTR_JPEG_S stJpegAttr;

	s32Ret = HI_MPI_VENC_CreateGroup(VencGrp);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_CreateGroup[%d] failed with %#x!\n", VencGrp, s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_CreateGroup[%d] failed with %#x!\n", VencGrp, s32Ret);
		Write_Log(0,log_msg);
		return -1;
	}

	stVencChnAttr.stVeAttr.enType = PT_JPEG;
	stJpegAttr.u32MaxPicWidth  = width;
	stJpegAttr.u32MaxPicHeight = height;
	stJpegAttr.u32PicWidth  = width;
	stJpegAttr.u32PicHeight = height;
	stJpegAttr.u32BufSize = stJpegAttr.u32MaxPicWidth * stJpegAttr.u32MaxPicHeight * 2;
	stJpegAttr.bByFrame = HI_TRUE;
	stJpegAttr.bVIField = HI_FALSE;
	stJpegAttr.u32Priority = 0;
	memcpy(&stVencChnAttr.stVeAttr.stAttrJpeg, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

	s32Ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_CreateChn [%d] faild with %#x!\n", VencChn, s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_CreateChn [%d] faild with %#x!\n", VencChn, s32Ret);
		Write_Log(0,log_msg);
		return -1;
	}

	s32Ret = HI_MPI_VENC_RegisterChn(VencGrp, VencChn);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_RegisterChn faild with %#x!\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_RegisterChn faild with %#x!\n", s32Ret);
		Write_Log(0,log_msg);
		return -1;
	}

    //
    // Bind VPSS & VENC
    //

    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = VpssGrp;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = VencGrp;
    stDestChn.s32ChnId = 0;

    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_SYS_Bind failed with %#x!\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_SYS_Bind failed with %#x!\n", s32Ret);
		Write_Log(0,log_msg);
        return -1;
    }

	s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
		Write_Log(0,log_msg);
		return -1;
	}

    return 0;
}

static int destroy_jpeg_encoder(int ch)
{
	char				log_msg[256];
    HI_S32 s32Ret = HI_SUCCESS;

    VENC_GRP VencGrp = ch;
    VENC_CHN VencChn = ch;
    VPSS_GRP VpssGrp = ch;
    VPSS_CHN VpssChn = ch;

	s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_StopRecvPic failed with %#x!\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_StopRecvPic failed with %#x!\n", s32Ret);
		Write_Log(0,log_msg);
		//return -1;
	}

    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = VpssGrp;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = VencGrp;
    stDestChn.s32ChnId = 0;

	s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_SYS_UnBind failed with %#x!\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_SYS_UnBind failed with %#x!\n", s32Ret);
		Write_Log(0,log_msg);
		//return -1;
	}

	s32Ret = HI_MPI_VENC_UnRegisterChn(VencChn);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_UnRegisterChn failed with %#x!\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_UnRegisterChn failed with %#x!\n", s32Ret);
		Write_Log(0,log_msg);
        //return -1;
	}

	s32Ret = HI_MPI_VENC_DestroyChn(VencChn);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_DestroyChn error :%x\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_DestroyChn error :%x\n", s32Ret);
		Write_Log(0,log_msg);
        //return -1;
	}

	s32Ret = HI_MPI_VENC_DestroyGroup(VencGrp);
	if(s32Ret != HI_SUCCESS) {
		//ob_error("HI_MPI_VENC_DestroyGroup error: %x\n", s32Ret);
		sprintf(log_msg,"[CODEC] I_MPI_VENC_DestroyGroup error: %x\n", s32Ret);
		Write_Log(0,log_msg);
        //return -1;
	}

    s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_StopGrp error=%d\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VPSS_StopGrp error=%d\n", s32Ret);
		Write_Log(0,log_msg);
        //return -1;
    }

    s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_DisableChn error=%d\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VPSS_DisableChn error=%d\n", s32Ret);
		Write_Log(0,log_msg);
        //return -1;
    }

    s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
    if(s32Ret != HI_SUCCESS) {
        //ob_error("HI_MPI_VPSS_DestroyGrp error=%d\n", s32Ret);
		sprintf(log_msg,"[CODEC] HI_MPI_VPSS_DestroyGrp error=%d\n", s32Ret);
		Write_Log(0,log_msg);
        //return -1;
    }

    return 0;
}

static int encode_jpeg(int ch, VIDEO_FRAME_INFO_S *frame, u8 *buf, int bsize)
{
	char				log_msg[256];
    HI_S32 s32Ret;
    VENC_CHN VencChn = ch;
    VPSS_GRP VpssGrp = ch;

	sprintf(log_msg,"[CODEC] width = %d height = %d top = %d , back = %d , left = %d , right = %d\n", frame->stVFrame.u32Width , frame->stVFrame.u32Height , frame->stVFrame.u16OffsetTop , frame->stVFrame.u16OffsetBottom , frame->stVFrame.u16OffsetLeft , frame->stVFrame.u16OffsetRight);
	Write_Log(0,log_msg);

#if 0 // direct to VENC
    s32Ret = HI_MPI_VENC_SendFrame(VencGrp, frame);
#else
    s32Ret = HI_MPI_VPSS_UserSendFrame(VpssGrp, frame);
#endif
    if(s32Ret != HI_SUCCESS) {
        //ob_error("send frame failed: %x\n", s32Ret);
		sprintf(log_msg,"[CODEC] send frame failed: %x\n", s32Ret);
		Write_Log(0,log_msg);
        return -1;
    }

    int fd;

	fd = HI_MPI_VENC_GetFd(VencChn);
	if(fd < 0) {
		//ob_error("HI_MPI_VENC_GetFd error, %d\n", fd);
		sprintf(log_msg,"[CODEC] HI_MPI_VENC_GetFd error, %d\n", fd);
		Write_Log(0,log_msg);
		return -1;
	}

    fd_set read_fds;
    struct timeval timeout;

	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	timeout.tv_sec  = 1;
	timeout.tv_usec = 0;

	int ret = select(fd+1, &read_fds, NULL, NULL, &timeout);
	if(ret < 0) {
		//ob_error("select failed! (%s)\n", strerror(errno));
		sprintf(log_msg,"[CODEC] select failed! (%s)\n", strerror(errno));
		Write_Log(0,log_msg);
		return -1;
	}
	else if(ret == 0) {
		//ob_error("time out!\n");
		sprintf(log_msg,"[CODEC] time out!\n");
		Write_Log(0,log_msg);
		return -1;
	}
	else {
        VENC_CHN_STAT_S stStat;
        VENC_STREAM_S stStream;
		if(FD_ISSET(fd, &read_fds)) {
			s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
			if(s32Ret != HI_SUCCESS) {
				//ob_error("HI_MPI_VENC_Query failed with %#x!\n", s32Ret);
				sprintf(log_msg,"[CODEC] HI_MPI_VENC_Query failed with %#x!\n", s32Ret);
				Write_Log(0,log_msg);
				return -1;
			}

			stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
			if(stStream.pstPack == NULL) {
				//ob_error("malloc memory failed!\n");
				sprintf(log_msg,"[CODEC] malloc memory failed!\n");
				Write_Log(0,log_msg);
				return -1;
			}

			stStream.u32PackCount = stStat.u32CurPacks;
			s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, HI_TRUE);
			if(s32Ret != HI_SUCCESS) {
				//ob_error("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
				sprintf(log_msg,"[CODEC] HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
				Write_Log(0,log_msg);
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				return -1;
			}

#if 0
			{
                static int out = 0;

                if(!out) {
                    FILE *fp = fopen("hi3531.jpg", "wb");
                    if(fp) {
                        int i;
                        for (i=0; i<stStream.u32PackCount; i++) {
                            fwrite(stStream.pstPack[i].pu8Addr[0], stStream.pstPack[i].u32Len[0], 1, fp);
                            fwrite(stStream.pstPack[i].pu8Addr[1], stStream.pstPack[i].u32Len[1], 1, fp);
                        }
                        fclose(fp);
                        out = 1;
                    }
                }
			}
#endif

            int i;
			int len = 0;
            int total = 0;
			for(i=0; i<stStream.u32PackCount; i++) {
				len += stStream.pstPack[i].u32Len[0];
				len += stStream.pstPack[i].u32Len[1];
			}

            //ob_debug("JPEG : %d bytes\n", len);

            if(buf) {
                if(len > bsize) {
                    //ob_error("oversized %d > %d\n", len, bsize);
					sprintf(log_msg,"[CODEC] oversized %d > %d\n", len, bsize);
					Write_Log(0,log_msg);
                }
                else {
                    len = 0;
                    for(i=0; i<stStream.u32PackCount; i++) {
                        if(stStream.pstPack[i].u32Len[0] > 0) {
                            memcpy(&buf[len], stStream.pstPack[i].pu8Addr[0], stStream.pstPack[i].u32Len[0]);
                            len += stStream.pstPack[i].u32Len[0];
                        }
                        if (stStream.pstPack[i].u32Len[1] > 0) {
                            memcpy(&buf[len], stStream.pstPack[i].pu8Addr[1], stStream.pstPack[i].u32Len[1]);
                            len += stStream.pstPack[i].u32Len[1];
                        }
                    }
                    total = len;
                }
            }

			s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
			if(s32Ret) {
				//ob_error("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
				sprintf(log_msg,"[CODEC] HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
				Write_Log(0,log_msg);
			}

			free(stStream.pstPack);
			stStream.pstPack = NULL;
            return len;
		}
	}
    return -1;
}

static int create_decoder(int ch, int is_h264)
{
	char				log_msg[256];
    VDEC_CHN_ATTR_S stAttr;
    HI_S32 s32Ret;

    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));

    stAttr.enType = (is_h264) ? PT_H264 : PT_JPEG;
    stAttr.u32BufSize = 1920*1080;
    stAttr.u32Priority = 1;
    stAttr.u32PicWidth = 1920;
    stAttr.u32PicHeight = 1080;

    switch (stAttr.enType) {
        case PT_H264:
            stAttr.stVdecVideoAttr.u32RefFrameNum = 1;
            stAttr.stVdecVideoAttr.enMode = VIDEO_MODE_FRAME;
            stAttr.stVdecVideoAttr.s32SupportBFrame = 0;
            break;
        case PT_JPEG:
            stAttr.stVdecJpegAttr.enMode = VIDEO_MODE_FRAME;
            break;
        case PT_MJPEG:
            stAttr.stVdecJpegAttr.enMode = VIDEO_MODE_FRAME;
            break;
        default:
            //ob_error("ch#%d unknown type\n", ch);
			sprintf(log_msg,"[CODEC] ch#%d unknown type\n", ch);
			Write_Log(0,log_msg);
            return -1;
    }

    s32Ret = HI_MPI_VDEC_CreateChn(ch, &stAttr);
    if (HI_SUCCESS != s32Ret) {
        //ob_error("ch#%d HI_MPI_VDEC_CreateChn failed errno 0x%x \n", ch, s32Ret);
		sprintf(log_msg,"[CODEC] ch#%d HI_MPI_VDEC_CreateChn failed errno 0x%x \n", ch, s32Ret);
		Write_Log(0,log_msg);
        return -1;
    }

    s32Ret = HI_MPI_VDEC_StartRecvStream(ch);
    if (HI_SUCCESS != s32Ret) {
        //ob_error("ch#%d HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", ch, s32Ret);
		sprintf(log_msg,"[CODEC] ch#%d HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", ch, s32Ret);
		Write_Log(0,log_msg);
        return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------------
// APIs
// ----------------------------------------------------------------------------
int codec_init(void)
{
    if(codec_sys_init() < 0)
        return -1;
    int i;
    for(i=0;i<CH_NR;i++) {
        create_decoder(i, 1);
        pthread_mutex_init(&c_mutex[i], 0);
    }

    return 0;
}

int codec_exit(void)
{
    return 0;
}

void sample_yuv_dump(VIDEO_FRAME_S * pVBuf, FILE *pfd,int id)
{
    unsigned int w, h;
	int	i,j;
	char				log_msg[256];
	//LPBYTE    lpByte_tmp;
	//LPBYTE	  lpByte3;
	RECT	  lpSrc[1];
    char * pVBufVirt_Y;
    char * pVBufVirt_C;
    char * pMemContent;
    unsigned char TmpBuff[1024];
    HI_U32 phy_addr,size;
	HI_CHAR *pUserPageAddr[2];
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;/* height of UV when planar format*/
	//lpByte_tmp = malloc(3000000);
	//lpByte3 = malloc(1000000);
		
	lpSrc[0].left = 0;
	lpSrc[0].bottom = pVBuf->u32Height/2-1;
	lpSrc[0].right = pVBuf->u32Width-1;
	lpSrc[0].top = pVBuf->u32Height/4-1;

    sprintf(log_msg,"[CODEC] width = %d , height = %d\n",pVBuf->u32Width,pVBuf->u32Height);
	Write_Log(0,log_msg);
    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
        size = ((pVBuf->u32Stride[0]))*(pVBuf->u32Height)*3/2;    
        u32UvHeight = pVBuf->u32Height/2;
    }
    else
    {
        size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*2;   
        u32UvHeight = pVBuf->u32Height;
    }

    phy_addr = pVBuf->u32PhyAddr[0];
    pUserPageAddr[0] = (HI_U8 *) HI_MPI_SYS_Mmap(phy_addr, size);	
    if (NULL == pUserPageAddr)
    {
        return ;
    }
	fwrite(pUserPageAddr[0], size, 1, pfd);
	//memcpy(lpByte_tmp,(BYTE*)pUserPageAddr[0],size);
	//for (i=0;i<pVBuf->u32Height;i++)
	//{
	//	for (j=0;j<640;j++)
	//	{
	//		lpByte3[640*i+j]=lpByte_tmp[1920*i+j];
	//		//printf("%u:%u ",lpByte2[j],lpByte_tmp[j]);
	//	}
	//		//printf("\n");
	//}

	//JRDetectionEdge(lpByte3,d_byte[id].lpByte2,640,480,lpSrc,2);
 //   fwrite(d_byte[id].lpByte2, 640*480, 1, pfd);
	//free(lpByte_tmp);
	//free(lpByte3);
/*
	pVBufVirt_Y = pUserPageAddr[0]; 
	pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0])*(pVBuf->u32Height);


    for(h=0; h<pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h*pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }
    fflush(pfd);


    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        pMemContent += 1;

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);
    
    for(h=0; h<u32UvHeight; h++)    
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);
  */  
    HI_MPI_SYS_Munmap(pUserPageAddr[0], size);    
}

int h264tojpeg(void *data, int size, void *out, int bsize, int x, int y, int width, int height,int flag,int id)
{
    int ch;
	int count = 0;
	char				log_msg[256];
	RECT				lpSrc[1];
	int					motion;
	int					jsize = 0;
	VDEC_STREAM_S vdec_stream;

	//if (height == 1080)
	//{
	//height = 1088;
	//}
	Write_Log(0,"[CODEC] Inside h264tojpeg \n");
	sprintf(log_msg,"[CODEC] 1 real data = (%x), real size = %d,Out data = (%x), Out size = %d,x = %d,y = %d,width = %d,height = %d\n",data , size,out, bsize, x, y,width,height);
	Write_Log(0,log_msg);
	vdec_stream.pu8Addr = (u8 *)data;
	vdec_stream.u32Len = size;
	vdec_stream.u64PTS = 0;
    pthread_mutex_lock(&i_mutex);
    ch = c_index;
    c_index = (c_index + 1) % CH_NR;
    pthread_mutex_unlock(&i_mutex);
	pthread_mutex_lock(&c_mutex[ch]);
	//while(1)
	//{
	//	sprintf(log_msg,"[CODEC] Try_lock fail! times = %d\n",count);
	//	Write_Log(0,log_msg);
	//	if (pthread_mutex_trylock(&c_mutex[ch]) == 0)
	//	{
	//		break;
	//	}
	//	count++;
	//	if (count ==5){return -1;}
	//}
    

	int ret = HI_MPI_VDEC_SendStream_TimeOut(ch, &vdec_stream, 1000);
	if (ret != HI_SUCCESS) {
        pthread_mutex_unlock(&c_mutex[ch]);
		//ob_error("[%d]send to vdec error (0x%x)\n", ch, ret);
		sprintf(log_msg,"[CODEC] [%d]send to vdec error (0x%x)\n", ch, ret);
		Write_Log(0,log_msg);
		return -1;
	}

    VIDEO_FRAME_INFO_S frame;
    memset(&frame, 0, sizeof(frame));
    //ret = HI_MPI_VDEC_GetImage(ch, &frame, HI_IO_BLOCK);
    ret = HI_MPI_VDEC_GetImage_TimeOut(ch, &frame, 8000);
	if (ret != HI_SUCCESS) {
        pthread_mutex_unlock(&c_mutex[ch]);
		//ob_error("[%d]get image error (0x%x)\n", ch, ret);
		sprintf(log_msg,"[CODEC] [%d]get image error (0x%x)\n", ch, ret);
		Write_Log(0,log_msg);
        VDEC_CHN_STAT_S stat;
        ret = HI_MPI_VDEC_Query(ch, &stat);
        if(ret != HI_SUCCESS) {
            //ob_error("[%d]get stat error (0x%x)\n", ch, ret);
			sprintf(log_msg,"[CODEC] [%d]get stat error (0x%x)\n", ch, ret);
			Write_Log(0,log_msg);
            return -1;
        }
        // ob_debug("[%d]STAT: %u bytes, %u frames, %u pics, %d, %u received, %u decoded\n", ch,
        //        stat.u32LeftStreamBytes, stat.u32LeftStreamFrames, stat.u32LeftPics,
        //        stat.bStartRecvStream, stat.u32RecvStreamFrames, stat.u32DecodeStreamFrames);
		sprintf(log_msg,"[CODEC] [%d]STAT: %u bytes, %u frames, %u pics, %d, %u received, %u decoded\n", ch,
                stat.u32LeftStreamBytes, stat.u32LeftStreamFrames, stat.u32LeftPics,
                stat.bStartRecvStream, stat.u32RecvStreamFrames, stat.u32DecodeStreamFrames);
		Write_Log(0,log_msg);

		return -1;
	}

#if 0
    void *virt1 = HI_MPI_SYS_Mmap(frame.stVFrame.u32PhyAddr[0], frame.stVFrame.u32Stride[0]*frame.stVFrame.u32Height);
    void *virt2 = HI_MPI_SYS_Mmap(frame.stVFrame.u32PhyAddr[1], frame.stVFrame.u32Stride[1]*frame.stVFrame.u32Height);

    //ob_debug("[%u] pixel=%d, wxh=%ux%u, %p,%p,%p, %08x,%08x,%08x, %u,%u,%u\n",
	sprintf(log_msg,"[CODEC] [%u] pixel=%d, wxh=%ux%u, %p,%p,%p, %08x,%08x,%08x, %u,%u,%u\n",
            frame.u32PoolId,
            frame.stVFrame.enPixelFormat,
            frame.stVFrame.u32Width,
            frame.stVFrame.u32Height,
#if 1
            virt1, virt2, (void *)0,
#else
            frame.stVFrame.pVirAddr[0],
            frame.stVFrame.pVirAddr[1],
            frame.stVFrame.pVirAddr[2],
#endif
            frame.stVFrame.u32PhyAddr[0],
            frame.stVFrame.u32PhyAddr[1],
            frame.stVFrame.u32PhyAddr[2],
            frame.stVFrame.u32Stride[0],
            frame.stVFrame.u32Stride[1],
            frame.stVFrame.u32Stride[2]);
		Write_Log(0,log_msg);
#endif
		if (flag ==0)
		{
			Write_Log(0,"flag = 0\n");
			if(create_jpeg_encoder(ch, x, y, width, height) == 0) {
				jsize = encode_jpeg(ch, &frame, (u8 *)out, bsize);
				if(jsize < 0) {
					//ob_error("[%d]encode jpeg error\n", ch);
					sprintf(log_msg,"[CODEC] [%d]encode jpeg error\n", ch);
					Write_Log(0,log_msg);
				}
			}
			else {
				//ob_error("[%d]create jpeg encoder error\n", ch);
				sprintf(log_msg,"[CODEC] [%d]create jpeg encoder error\n", ch);
				Write_Log(0,log_msg);
			}

			destroy_jpeg_encoder(ch);

			ret = HI_MPI_VDEC_ReleaseImage(ch, &frame);
			if (ret != HI_SUCCESS) {
				//ob_error("[%d]release image error (0x%x)\n", ch, ret);
				sprintf(log_msg,"[CODEC] [%d]release image error (0x%x)\n", ch, ret);
				Write_Log(0,log_msg);
			}

			pthread_mutex_unlock(&c_mutex[ch]);

			return jsize;
		}
		else
		{
			FILE *fp ;
			fp = fopen("test.yuv","wb");
			if(fp)
			{
				sample_yuv_dump(&frame,fp,id);
				fclose(fp);
			}
			lpSrc[0].left = 0;
			lpSrc[0].bottom = (height/4*3)-1;
			lpSrc[0].right = width-1;
			lpSrc[0].top = height/4-1;
			motion = _GrayMotionRate(d_byte[id].lpByte,width,d_byte[id].lpByte2,width,height,lpSrc);
			sprintf(log_msg,"[CODEC] [ch%02d] OUT motion = %d\n",id, motion);
			Write_Log(0,log_msg);
			//qwe++;
			if (motion>=10)
			{
				memcpy(d_byte[id].lpByte,d_byte[id].lpByte2,(height*width*3/2));
				sprintf(log_msg,"[CODEC] motion = %d ,lpByte = (%x),lpByte2 = (%x) \n", motion,d_byte[id].lpByte,d_byte[id].lpByte2);
				Write_Log(0,log_msg);
				if(create_jpeg_encoder(ch, x, y, width, height) == 0) {
					jsize = encode_jpeg(ch, &frame, (u8 *)out, bsize);
					if(jsize < 0) {
						//ob_error("[%d]encode jpeg error\n", ch);
						sprintf(log_msg,"[CODEC] [%d]encode jpeg error\n", ch);
						Write_Log(0,log_msg);
					}
				}
				else {
					//ob_error("[%d]create jpeg encoder error\n", ch);
					sprintf(log_msg,"[CODEC] [%d]create jpeg encoder error\n", ch);
					Write_Log(0,log_msg);
				}

				destroy_jpeg_encoder(ch);

				ret = HI_MPI_VDEC_ReleaseImage(ch, &frame);
				if (ret != HI_SUCCESS) {
					//ob_error("[%d]release image error (0x%x)\n", ch, ret);
					sprintf(log_msg,"[CODEC] [%d]release image error (0x%x)\n", ch, ret);
					Write_Log(0,log_msg);
				}

				pthread_mutex_unlock(&c_mutex[ch]);

				//FILE *fp1;
				//char mpath[64];
				//sprintf(mpath,"tmp%d.jpg",qwe);
				//if(fp1 = fopen(mpath,"wb"))
				//{
				//	fwrite(out,jsize,1,fp1);
				//	fclose(fp1);
				//}
				return jsize;
			} 
			else
			{
				memcpy(d_byte[id].lpByte,d_byte[id].lpByte2,(height*width*3/2));
				ret = HI_MPI_VDEC_ReleaseImage(ch, &frame);
				if (ret != HI_SUCCESS) {
					//ob_error("[%d]release image error (0x%x)\n", ch, ret);
					sprintf(log_msg,"[CODEC] [%d]release image error (0x%x)\n", ch, ret);
					Write_Log(0,log_msg);
				}
				pthread_mutex_unlock(&c_mutex[ch]);
				return 0;
			}
		}
}

