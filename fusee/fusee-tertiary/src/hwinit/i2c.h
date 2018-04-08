#ifndef _I2C_H_
#define _I2C_H_

#include "types.h"

#define I2C_1 0
#define I2C_2 1
#define I2C_3 2
#define I2C_4 3
#define I2C_5 4
#define I2C_6 5

void i2c_init(u32 idx);
int i2c_send_buf_small(u32 idx, u32 x, u32 y, u8 *buf, u32 size);
int i2c_recv_buf_small(u8 *buf, u32 size, u32 idx, u32 x, u32 y);
void i2c_send_byte(u32 idx, u32 x, u32 y, u8 b);
u8 i2c_recv_byte(u32 idx, u32 x, u32 y);

#endif
