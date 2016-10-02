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
#include "ob_platform.h"
#include "OBWinDef.h"
#include "OBErrMsg.h"
#include "OBLogFil.h"
#include "OBImgPrc.h"
#include "CVWnLPR.h"
#include "CP_LprFrm.h"

// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------
#define LOG_TAG     "LPR"
#define LOG_DEBUG   1
#define LOG_INFO    1
#define LOG_TRACE   1
#include "debug.h"

// ----------------------------------------------------------------------------
// Constant & Macro Definition
// ----------------------------------------------------------------------------
#define LPR_MAGIC       0x4f6c5072      // 'OlPr'
#define DEFAULT_WIDTH   320
#define DEFAULT_HEIGHT  240

// ----------------------------------------------------------------------------
// Type Declaration
// ----------------------------------------------------------------------------
typedef struct {

    u32     magic;
	DWORD	dwRegion;
    HANDLE  hLic;
    HANDLE  hLpr;
	HIMAGE	hImage;
    u16     width;
    u16     height;
    void    *extPtr;
    char    Msg[MAX_PATH];
	PICRST	PicRst;

    HLOG    hLog;

} lpr_ctrl_t;

// ----------------------------------------------------------------------------
// Local Variable Declaration
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Local Functions
// ----------------------------------------------------------------------------
void lpr_free(void *handle)
{
    lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;

    if(lc->magic != LPR_MAGIC) {
        ob_error("Wrong MAGIC\n");
        return ;
    }

    if(lc->hLog) {
        FreeLog(lc->hLog);
        lc->hLog = NULL;
    }

    if(lc->hImage) {
        if(lc->extPtr) {
            ImageUnhookBuffer(lc->hImage);
            // free buffer here ?
        }
        OCR_FreeImage(lc->hImage);
        lc->hImage = NULL;
    }

    if(lc->hLic) {
        if(!WNLPRClose(lc->hLic))
            ShowError();
        lc->hLic = NULL;
    }
    lc->magic = 0;
}

void *lpr_alloc(short width, short height)
{
    lpr_ctrl_t *lc = (lpr_ctrl_t *)malloc(sizeof(lpr_ctrl_t));
    if(!lc) {
        ob_error("Out of memory\n");
        return 0;
    }

    memset(lc, 0, sizeof(lpr_ctrl_t));
    lc->magic = LPR_MAGIC;
    lc->dwRegion = 1;
    lc->hLic = WNLPRInitEx(LPR_LIC_CR20, NULL, lc->dwRegion);
	if(lc->hLic == NULL) {
		ShowError();
        lpr_free(lc);
		return 0;
	}

    //lc->hLog = CreateLog("./obnvr.log");
    //ob_debug("hLog = %p\n", lc->hLog);

    int nStatus;
    //lc->hLpr = WNLPRCreateEx2(lc->hLic, width, height, "./", &nStatus, NULL, lc->hLog);
    lc->hLpr = WNLPRCreateEx2(lc->hLic, 1280, 720, "./", &nStatus, NULL, NULL);
    if(lc->hLpr == NULL) {
		ShowError();
        lpr_free(lc);
		return 0;
	}

	sprintf( lc->Msg, "%d", 8 );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_EDGE, lc->Msg) )
		ShowError();
	sprintf( lc->Msg, "%d",  );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_LOCMTHD, lc->Msg) )
		ShowError();
	sprintf( lc->Msg, "%d", LPR_PRM_TWN_FMT_CAR|LPR_PRM_TWN_FMT_PTAXI );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PFORMAT, lc->Msg) )
		ShowError();
	sprintf( lc->Msg, "%d", 9 );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PEXPLVL, lc->Msg) )
		ShowError();
	sprintf( lc->Msg, "%d", 6 );
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PLATENO, lc->Msg) )
		ShowError();
	//if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PWIDTH, "138") )
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PWIDTH, "110") )
		ShowError();
	//if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PHEIGHT, "62") )
	if( !WNLPRSetParam(lc->hLpr, LPR_PRM_PHEIGHT, "50") )
		ShowError();

    lc->hImage = OCR_CreateImage(width, width, height, 8, TRUE);
    if(lc->hImage == NULL) {
		ShowError();
        lpr_free(lc);
        return 0;
    }
    lc->width = width;
    lc->height = height;

	return lc;
}

int lpr_process(void *handle, void *buf)
{
    lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;

    if(lc->magic != LPR_MAGIC || !buf)
        return -1;

    if(!ImageHookBuffer(lc->hImage, (LPSTR)buf)) {
        ShowError();
        return -1;
    }

    if(!WNLPRRecog1(lc->hLpr, lc->hImage, &lc->PicRst)) {
        ShowError();
        return -1;
    }

#if 1
    {
        FILE *fp = fopen("out.y", "wb");
        if(fp) {
            fwrite(buf, 1, 320*240, fp);
            fclose(fp);
        }
    }
#endif

#if 0
    int i;
    for(i=0; i<lc->PicRst.nPlateNo; i++)
        ob_info( "\t\tP#%d: %s@(%d,%d)(%d,%d)\n", i+1,
                lc->PicRst.PltRst[i].szCarNo,
                lc->PicRst.PltRst[i].rectLoc.left, lc->PicRst.PltRst[i].rectLoc.top,
                lc->PicRst.PltRst[i].rectLoc.right, lc->PicRst.PltRst[i].rectLoc.bottom );
#endif

    ImageUnhookBuffer(lc->hImage);

    return 0;
}

int lpr_dump_result(void *handle)
{
    lpr_ctrl_t *lc = (lpr_ctrl_t *)handle;

    if(lc->magic != LPR_MAGIC)
        return -1;

    ob_info("# of results: %d\n", lc->PicRst.nPlateNo);

    int i;
    for(i=0; i<lc->PicRst.nPlateNo; i++)
        ob_info( "\t\tP#%d: %s@(%d,%d)(%d,%d)\n", i+1,
                lc->PicRst.PltRst[i].szCarNo,
                lc->PicRst.PltRst[i].rectLoc.left, lc->PicRst.PltRst[i].rectLoc.top,
                lc->PicRst.PltRst[i].rectLoc.right, lc->PicRst.PltRst[i].rectLoc.bottom );

    ImageUnhookBuffer(lc->hImage);

    return 0;
}

