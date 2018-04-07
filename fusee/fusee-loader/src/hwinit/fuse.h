#ifndef _FUSE_H_
#define _FUSE_H_

#include "types.h"

/*! Fuse registers. */
#define FUSE_CTRL 0x0
#define FUSE_ADDR 0x4
#define FUSE_RDATA 0x8
#define FUSE_WDATA 0xC
#define FUSE_TIME_RD1 0x10
#define FUSE_TIME_RD2 0x14
#define FUSE_TIME_PGM1 0x18
#define FUSE_TIME_PGM2 0x1C
#define FUSE_PRIV2INTFC 0x20
#define FUSE_FUSEBYPASS 0x24
#define FUSE_PRIVATEKEYDISABLE 0x28
#define FUSE_DISABLEREGPROGRAM 0x2C
#define FUSE_WRITE_ACCESS_SW 0x30
#define FUSE_PWR_GOOD_SW 0x34

/*! Fuse cache registers. */
#define FUSE_RESERVED_ODMX(x) (0x1C8 + 4 * (x))

void fuse_disable_program();
u32 fuse_read_odm(u32 idx);

#endif
