
#ifndef __MCU_H__
#define __MCU_H__

//                          int id, int value
typedef int (*mcu_callback_t) (int, int);
#define MCU_EV_ALARMIN      1
#define MCU_EV_FP           2

extern int mcu_init(mcu_callback_t cb);
extern int mcu_exit(void);

extern int mcu_alarmout(u8 data);
extern int mcu_buzzer(int on);
extern int mcu_wdt_reset(int sec);
extern int mcu_wdt_disable(void);
extern int mcu_poweroff(void);

#endif // __MCU_H__

