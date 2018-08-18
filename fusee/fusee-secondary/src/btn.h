#ifndef FUSEE_BTN_H_
#define FUSEE_BTN_H_

#define BTN_POWER 0x1
#define BTN_VOL_DOWN 0x2
#define BTN_VOL_UP 0x4

uint32_t btn_read();
uint32_t btn_wait();
uint32_t btn_wait_timeout(uint32_t time_ms, uint32_t mask);

#endif