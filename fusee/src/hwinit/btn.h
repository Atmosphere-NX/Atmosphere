#ifndef _BTN_H_
#define _BTN_H_

#include "types.h"

#define BTN_POWER 0x1
#define BTN_VOL_DOWN 0x2
#define BTN_VOL_UP 0x4

u32 btn_read();
u32 btn_wait();

#endif
