#include "dbg_log.h"
#include "ob_platform.h"

int main(int argc, char **argv)
{
	struct timeval tt;
	int sum=0;
	gettimeofday(&tt, 0);
	u32 now = (u32) tt.tv_sec;
	while(1){
		Write_Log(sum,"123\n");
		//Write_Log(0,"345\n");
		sum++;
		sleep(1);
		//now=now+3600;
	}
	return 0;
}