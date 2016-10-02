#include "ob_platform.h"
#include "lpr.h"

#include "OBWinDef.h"
#include "OBImgPrc.h"
#include "OBErrMsg.h"

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
	result_info *result;
	HIMAGE	hImage;
	LPSTR	bufer;
	//lpr_ctrl_t *lpr;
	int ret,i;
	int id = 0;
	char path[64];
	
	//void *ptr = malloc(320*240);
	//if(!ptr) {
	//    dbg("OOM\n");
	//}

    void *lpr = lpr_alloc(1,result,"3531.ini");
    if(!lpr) {
        dbg("LPR alloc failed\n");
        exit(-1);
    }	
	for (i=0;i<32;i++)
	{
		sprintf(path,"Before-%02d.obi",i);
		if ((hImage = OCR_LoadImage(path,0,0)) == NULL)
		{
			break;
		}

		clock_t	start, finish;
		start = clock();
		ret = lpr_process(lpr, hImage,0,0);
		finish = clock();

		if(ret == 0) {
			u32 elapsed = (u32)(finish - start) * 1000 / CLOCKS_PER_SEC;
			dbg("LPR takes %u msec\n", elapsed);
			//lpr_dump_result(lpr);
		}
		else {
			dbg("LPR failed\n");
		}

		//lpr_free(lpr);
	}
	


	return 0;
}

