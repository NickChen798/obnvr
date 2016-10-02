#include "ob_platform.h"
#include "cexcept.h"
#include "OBWinDef.h"
#include "OBErrMsg.h"
#include "OBLogFil.h"
#include "OBImgPrc.h"
#include "CVWnLPR.h"
#include "CP_LprFrm.h"
#include "lpr.h"
#include "dbg_msg/dbg_log.h"

#include <semaphore.h>

int main(int argc, char **argv)
{
    //if(argc < 2) {
    //    dbg("Usage : %s FILE\n", argv[0]);
    //    return 0;
    //}

    //FILE *fp = fopen(argv[1], "rb");
    //if(!fp) {
    //    dbg("%s open failure\n", argv[1]);
    //    return 0;
    //}



    //int ret = fread(ptr, 1, 320*240, fp);
    //if(ret != (320*240)) {
    //    dbg("file size mismatch :%d\n", ret);
    //    return 0;
    //}
	result_info			result[MAX_CH];
	result_info			voting_ans;
	BYTE				tmp_buf[76800];
	u8					*image_buf;
	HIMAGE	hImage;
	LPSTR	bufer;
	//lpr_ctrl_t *lpr;
	int ret,i;
	int id = 0;
	char path[64];
	char				log_msg[256];
	
	//void *ptr = malloc(320*240);
	//if(!ptr) {
	//    dbg("OOM\n");
	//}
    void *lpr = lpr_alloc(1,result,"3531.ini");
    if(!lpr) {
		sprintf(log_msg,"[LPR] LPR alloc failed\n");
		Write_Log(0,log_msg);
        exit(-1);
    }	
	for (i=0;i<1;i++)
	{
		//File2RAW(path,320,240,8,0);
		sprintf(path,"test-4.obi");
		if ((hImage = OCR_LoadImage(path,0,0)) == NULL)
		{
			break;
		}
		//FILE *fp = fopen(path, "rb");
		//if (fp)
		//{
		//	//fread(hImage,1,20,fp);
		//	fread(image_buf, 1, 320*240, fp);
		//	fclose(fp);
		//}	
		clock_t	start, finish;
		start = clock();
		ret = lpr_process(lpr, hImage,0,0);
		finish = clock();

		OCR_WriteImage("test_bak.obi",hImage,IMG_FMT_OBI,0);

		if(ret == 0) {
			u32 elapsed = (u32)(finish - start) * 1000 / CLOCKS_PER_SEC;
			sprintf(log_msg,"%d. LPR takes %u msec\n",i, elapsed);
			Write_Log(0,log_msg);
			lpr_dump_result(lpr,&(voting_ans),tmp_buf);
		}
		else {
			sprintf(log_msg,"[LPR] LPR failed\n");
			Write_Log(0,log_msg);
		}
	}
		lpr_free(lpr);	


	return 0;
}

