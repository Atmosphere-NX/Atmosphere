/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
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

#ifndef _PMC_H_
#define _PMC_H_

/*! PMC registers. */
#define APBDEV_PMC_CNTRL 0x0
#define  PMC_CNTRL_MAIN_RST (1 << 4)
#define APBDEV_PMC_SEC_DISABLE 0x4
#define APBDEV_PMC_PWRGATE_TOGGLE 0x30
#define APBDEV_PMC_PWRGATE_STATUS 0x38
#define APBDEV_PMC_NO_IOPOWER 0x44
#define  PMC_NO_IOPOWER_SDMMC1_IO_EN (1 << 12)
#define APBDEV_PMC_SCRATCH0 0x50
#define APBDEV_PMC_SCRATCH1 0x54
#define APBDEV_PMC_SCRATCH20 0xA0
#define APBDEV_PMC_PWR_DET_VAL 0xE4
#define  PMC_PWR_DET_SDMMC1_IO_EN (1 << 12)
#define APBDEV_PMC_DDR_PWR 0xE8
#define APBDEV_PMC_CRYPTO_OP 0xF4
#define  PMC_CRYPTO_OP_SE_ENABLE  0
#define  PMC_CRYPTO_OP_SE_DISABLE 1
#define APBDEV_PMC_SCRATCH33 0x120
#define APBDEV_PMC_SCRATCH40 0x13C
#define APBDEV_PMC_OSC_EDPD_OVER 0x1A4
#define  PMC_OSC_EDPD_OVER_OSC_CTRL_OVER 0x400000
#define APBDEV_PMC_RST_STATUS 0x1B4
#define APBDEV_PMC_IO_DPD_REQ 0x1B8
#define APBDEV_PMC_IO_DPD2_REQ 0x1C0
#define APBDEV_PMC_VDDP_SEL 0x1CC
#define APBDEV_PMC_DDR_CFG 0x1D0
#define APBDEV_PMC_SCRATCH45 0x234
#define APBDEV_PMC_SCRATCH46 0x238
#define APBDEV_PMC_SCRATCH49 0x244
#define APBDEV_PMC_TSC_MULT 0x2B4
#define APBDEV_PMC_SEC_DISABLE2 0x2C4
#define APBDEV_PMC_WEAK_BIAS 0x2C8
#define APBDEV_PMC_REG_SHORT 0x2CC
#define APBDEV_PMC_SEC_DISABLE3 0x2D8
#define APBDEV_PMC_SECURE_SCRATCH21 0x334
#define  PMC_FUSE_PRIVATEKEYDISABLE_TZ_STICKY_BIT 0x10
#define APBDEV_PMC_SECURE_SCRATCH32 0x360
#define APBDEV_PMC_SECURE_SCRATCH49 0x3A4
#define APBDEV_PMC_CNTRL2 0x440
#define  PMC_CNTRL2_HOLD_CKE_LOW_EN 0x1000
#define APBDEV_PMC_IO_DPD3_REQ 0x45C
#define APBDEV_PMC_IO_DPD4_REQ 0x464
#define APBDEV_PMC_UTMIP_PAD_CFG1 0x4C4
#define APBDEV_PMC_UTMIP_PAD_CFG3 0x4CC
#define APBDEV_PMC_DDR_CNTRL 0x4E4
#define APBDEV_PMC_SEC_DISABLE4 0x5B0
#define APBDEV_PMC_SEC_DISABLE5 0x5B4
#define APBDEV_PMC_SEC_DISABLE6 0x5B8
#define APBDEV_PMC_SEC_DISABLE7 0x5BC
#define APBDEV_PMC_SEC_DISABLE8 0x5C0
#define APBDEV_PMC_SCRATCH188 0x810
#define APBDEV_PMC_SCRATCH190 0x818
#define APBDEV_PMC_SCRATCH200 0x840

#endif
