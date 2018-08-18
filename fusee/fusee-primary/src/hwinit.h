#ifndef FUSEE_HWINIT_H_
#define FUSEE_HWINIT_H_

#define I2S_BASE 0x702D1000
#define MAKE_I2S_REG(n) MAKE_REG32(I2S_BASE + n)

void nx_hwinit();

#endif
