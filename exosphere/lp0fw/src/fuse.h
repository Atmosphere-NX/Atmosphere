/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef EXOSPHERE_WARMBOOT_BIN_FUSE_H
#define EXOSPHERE_WARMBOOT_BIN_FUSE_H

#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

typedef struct {
    uint32_t FUSE_CTRL;
    uint32_t FUSE_REG_ADDR;
    uint32_t FUSE_REG_READ;
    uint32_t FUSE_REG_WRITE;
    uint32_t FUSE_TIME_RD1;
    uint32_t FUSE_TIME_RD2;
    uint32_t FUSE_TIME_PGM1;
    uint32_t FUSE_TIME_PGM2;
    uint32_t FUSE_PRIV2INTFC;
    uint32_t FUSE_FUSEBYPASS;
    uint32_t FUSE_PRIVATEKEYDISABLE;
    uint32_t FUSE_DIS_PGM;
    uint32_t FUSE_WRITE_ACCESS;
    uint32_t FUSE_PWR_GOOD_SW;
    uint32_t _0x38[0x32];
} fuse_registers_t;

typedef struct {
    uint32_t FUSE_PRODUCTION_MODE;
    uint32_t _0x4;
    uint32_t _0x8;
    uint32_t _0xC;
    uint32_t FUSE_SKU_INFO;
    uint32_t FUSE_CPU_SPEEDO_0;
    uint32_t FUSE_CPU_IDDQ;
    uint32_t _0x1C;
    uint32_t _0x20;
    uint32_t _0x24;
    uint32_t FUSE_FT_REV;
    uint32_t FUSE_CPU_SPEEDO_1;
    uint32_t FUSE_CPU_SPEEDO_2;
    uint32_t FUSE_SOC_SPEEDO_0;
    uint32_t FUSE_SOC_SPEEDO_1;
    uint32_t FUSE_SOC_SPEEDO_2;
    uint32_t FUSE_SOC_IDDQ;
    uint32_t _0x44;
    uint32_t FUSE_FA;
    uint32_t _0x4C;
    uint32_t _0x50;
    uint32_t _0x54;
    uint32_t _0x58;
    uint32_t _0x5C;
    uint32_t _0x60;
    uint32_t FUSE_PUBLIC_KEY[0x8];
    uint32_t FUSE_TSENSOR_1;
    uint32_t FUSE_TSENSOR_2;
    uint32_t _0x8C;
    uint32_t FUSE_CP_REV;
    uint32_t _0x94;
    uint32_t FUSE_TSENSOR_0;
    uint32_t FUSE_FIRST_BOOTROM_PATCH_SIZE_REG;
    uint32_t FUSE_SECURITY_MODE;
    uint32_t FUSE_PRIVATE_KEY[0x4];
    uint32_t FUSE_DEVICE_KEY;
    uint32_t _0xB8;
    uint32_t _0xBC;
    uint32_t FUSE_RESERVED_SW;
    uint32_t FUSE_VP8_ENABLE;
    uint32_t FUSE_RESERVED_ODM[0x8];
    uint32_t _0xE8;
    uint32_t _0xEC;
    uint32_t FUSE_SKU_USB_CALIB;
    uint32_t FUSE_SKU_DIRECT_CONFIG;
    uint32_t _0xF8;
    uint32_t _0xFC;
    uint32_t FUSE_VENDOR_CODE;
    uint32_t FUSE_FAB_CODE;
    uint32_t FUSE_LOT_CODE_0;
    uint32_t FUSE_LOT_CODE_1;
    uint32_t FUSE_WAFER_ID;
    uint32_t FUSE_X_COORDINATE;
    uint32_t FUSE_Y_COORDINATE;
    uint32_t _0x11C;
    uint32_t _0x120;
    uint32_t FUSE_SATA_CALIB;
    uint32_t FUSE_GPU_IDDQ;
    uint32_t FUSE_TSENSOR_3;
    uint32_t _0x130;
    uint32_t _0x134;
    uint32_t _0x138;
    uint32_t _0x13C;
    uint32_t _0x140;
    uint32_t _0x144;
    uint32_t FUSE_OPT_SUBREVISION;
    uint32_t _0x14C;
    uint32_t _0x150;
    uint32_t FUSE_TSENSOR_4;
    uint32_t FUSE_TSENSOR_5;
    uint32_t FUSE_TSENSOR_6;
    uint32_t FUSE_TSENSOR_7;
    uint32_t FUSE_OPT_PRIV_SEC_DIS;
    uint32_t FUSE_PKC_DISABLE;
    uint32_t _0x16C;
    uint32_t _0x170;
    uint32_t _0x174;
    uint32_t _0x178;
    uint32_t _0x17C;
    uint32_t FUSE_TSENSOR_COMMON;
    uint32_t _0x184;
    uint32_t _0x188;
    uint32_t _0x18C;
    uint32_t _0x190;
    uint32_t _0x194;
    uint32_t _0x198;
    uint32_t FUSE_DEBUG_AUTH_OVERRIDE;
    uint32_t _0x1A0;
    uint32_t _0x1A4;
    uint32_t _0x1A8;
    uint32_t _0x1AC;
    uint32_t _0x1B0;
    uint32_t _0x1B4;
    uint32_t _0x1B8;
    uint32_t _0x1BC;
    uint32_t _0x1D0;
    uint32_t FUSE_TSENSOR_8;
    uint32_t _0x1D8;
    uint32_t _0x1DC;
    uint32_t _0x1E0;
    uint32_t _0x1E4;
    uint32_t _0x1E8;
    uint32_t _0x1EC;
    uint32_t _0x1F0;
    uint32_t _0x1F4;
    uint32_t _0x1F8;
    uint32_t _0x1FC;
    uint32_t _0x200;
    uint32_t FUSE_RESERVED_CALIB;
    uint32_t _0x208;
    uint32_t _0x20C;
    uint32_t _0x210;
    uint32_t _0x214;
    uint32_t _0x218;
    uint32_t FUSE_TSENSOR_9;
    uint32_t _0x220;
    uint32_t _0x224;
    uint32_t _0x228;
    uint32_t _0x22C;
    uint32_t _0x230;
    uint32_t _0x234;
    uint32_t _0x238;
    uint32_t _0x23C;
    uint32_t _0x240;
    uint32_t _0x244;
    uint32_t _0x248;
    uint32_t _0x24C;
    uint32_t FUSE_USB_CALIB_EXT;
    uint32_t _0x254;
    uint32_t _0x258;
    uint32_t _0x25C;
    uint32_t _0x260;
    uint32_t _0x264;
    uint32_t _0x268;
    uint32_t _0x26C;
    uint32_t _0x270;
    uint32_t _0x274;
    uint32_t _0x278;
    uint32_t _0x27C;
    uint32_t FUSE_SPARE_BIT[0x20];
} fuse_chip_registers_t;

#define FUSE_REGS       ((volatile fuse_registers_t *)(0x7000F800))
#define FUSE_CHIP_REGS  ((volatile fuse_chip_registers_t *)(0x7000F900))

#define MAKE_FUSE_REG(n) MAKE_REG32(0x7000F800 + n)

typedef struct {
    uint32_t offset;
    uint32_t value;
} fuse_bypass_data_t;

bool fuse_check_downgrade_status(void);

void fuse_configure_fuse_bypass(void);

void fuse_disable_programming(void);

#endif
