#include "ob_platform.h"
#include "mcu/mcu.h"

#define WDT_ENABLE
#define WDT_MARGIN_TIME		15

static void quit_signal(int sig)
{
// 	static int disable_flag = 0;
// 
// 	if (!disable_flag) {
// 		if (mcu_wdt_disable() < 0) {
// 			dbg("wdt_disable error\n");
// 		}
//         else {
//             dbg("WDT disabled\n");
//         }
// 
// 		disable_flag = 1;
// 	}
// 
// 	usleep(100000);

	exit(1);
}

static int mcu_callback(int id, int data)
{
    dbg("CB: id=%d, data=0x%02x\n", id, data);
	/*if(id == MCU_EV_FP && (data & 1) == 0) {
            dbg("PowerOff\n");
	    mcu_poweroff();
    	}*/
    return 0;
}

int main(int argc, char **argv)
{
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);

    mcu_init(mcu_callback);

	//mcu_alarmout(1);

	//mcu_alarmout(8);

    //sleep(5);

//#if 0 // PowerOff Test
//    dbg("PowerOff\n");
//
//    mcu_poweroff();
//#endif
//
//#if 0 // WDT Trigger Test
//    dbg("WDT Test\n");
//
//    mcu_wdt_reset(5);
//
//    sleep(3);
//
//    dbg("WDT Reset\n");
//
//    mcu_wdt_reset(5);
//
//    dbg("Exit\n");
//#endif

	int x=0;
	printf("Set watchdog .\n");
	//mcu_wdt_reset(15);

	while (x<100) {

//#ifdef WDT_ENABLE
#if 1

		if (mcu_wdt_reset(WDT_MARGIN_TIME) < 0) {
            dbg("WDT reset error\n");
		}
        else {
            dbg("WDT reset\n");
        }

		x = 1;
		printf("Set watchdog set %d times.\n",x);
#endif

		sleep(1);
	}

	return 0;
}

