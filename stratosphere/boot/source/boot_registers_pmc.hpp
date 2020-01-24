/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#pragma once
#include <stratosphere.hpp>

static constexpr uintptr_t PmcBase = 0x7000E400ul;

static constexpr size_t APBDEV_PMC_CNTRL = 0x0;
static constexpr u32 PMC_CNTRL_MAIN_RST = (1 << 4);
static constexpr size_t APBDEV_PMC_SEC_DISABLE = 0x4;
static constexpr size_t APBDEV_PMC_WAKE_MASK = 0xC;
static constexpr size_t APBDEV_PMC_WAKE_LVL = 0x10;
static constexpr size_t APBDEV_PMC_DPD_PADS_ORIDE = 0x1C;
static constexpr size_t APBDEV_PMC_PWRGATE_TOGGLE = 0x30;
static constexpr size_t APBDEV_PMC_PWRGATE_STATUS = 0x38;
static constexpr size_t APBDEV_PMC_BLINK_TIMER = 0x40;
static constexpr size_t APBDEV_PMC_NO_IOPOWER = 0x44;
static constexpr size_t APBDEV_PMC_SCRATCH0 = 0x50;
static constexpr size_t APBDEV_PMC_SCRATCH1 = 0x54;
static constexpr size_t APBDEV_PMC_SCRATCH20 = 0xA0;
static constexpr size_t APBDEV_PMC_AUTO_WAKE_LVL_MASK = 0xDC;
static constexpr size_t APBDEV_PMC_PWR_DET_VAL = 0xE4;
static constexpr u32 PMC_PWR_DET_SDMMC1_IO_EN = (1 << 12);
static constexpr size_t APBDEV_PMC_DDR_PWR = 0xE8;
static constexpr size_t APBDEV_PMC_CRYPTO_OP = 0xF4;
static constexpr u32 PMC_CRYPTO_OP_SE_ENABLE  = 0;
static constexpr u32 PMC_CRYPTO_OP_SE_DISABLE = 1;
static constexpr size_t APBDEV_PMC_SCRATCH33 = 0x120;
static constexpr size_t APBDEV_PMC_SCRATCH40 = 0x13C;
static constexpr size_t APBDEV_PMC_WAKE2_MASK = 0x160;
static constexpr size_t APBDEV_PMC_WAKE2_LVL = 0x164;
static constexpr size_t APBDEV_PMC_AUTO_WAKE2_LVL_MASK = 0x170;
static constexpr size_t APBDEV_PMC_OSC_EDPD_OVER = 0x1A4;
static constexpr size_t APBDEV_PMC_CLK_OUT_CNTRL = 0x1A8;
static constexpr size_t APBDEV_PMC_RST_STATUS = 0x1B4;
static constexpr size_t APBDEV_PMC_IO_DPD_REQ = 0x1B8;
static constexpr size_t APBDEV_PMC_IO_DPD2_REQ = 0x1C0;
static constexpr size_t APBDEV_PMC_VDDP_SEL = 0x1CC;
static constexpr size_t APBDEV_PMC_DDR_CFG = 0x1D0;
static constexpr size_t APBDEV_PMC_SCRATCH45 = 0x234;
static constexpr size_t APBDEV_PMC_SCRATCH46 = 0x238;
static constexpr size_t APBDEV_PMC_SCRATCH49 = 0x244;
static constexpr size_t APBDEV_PMC_TSC_MULT = 0x2B4;
static constexpr size_t APBDEV_PMC_SEC_DISABLE2 = 0x2C4;
static constexpr size_t APBDEV_PMC_WEAK_BIAS = 0x2C8;
static constexpr size_t APBDEV_PMC_REG_SHORT = 0x2CC;
static constexpr size_t APBDEV_PMC_SEC_DISABLE3 = 0x2D8;
static constexpr size_t APBDEV_PMC_SECURE_SCRATCH21 = 0x334;
static constexpr size_t APBDEV_PMC_SECURE_SCRATCH32 = 0x360;
static constexpr size_t APBDEV_PMC_SECURE_SCRATCH49 = 0x3A4;
static constexpr size_t APBDEV_PMC_CNTRL2 = 0x440;
static constexpr size_t APBDEV_PMC_IO_DPD3_REQ = 0x45C;
static constexpr size_t APBDEV_PMC_IO_DPD4_REQ = 0x464;
static constexpr size_t APBDEV_PMC_UTMIP_PAD_CFG1 = 0x4C4;
static constexpr size_t APBDEV_PMC_UTMIP_PAD_CFG3 = 0x4CC;
static constexpr size_t APBDEV_PMC_WAKE_DEBOUNCE_EN = 0x4D8;
static constexpr size_t APBDEV_PMC_DDR_CNTRL = 0x4E4;
static constexpr size_t APBDEV_PMC_SEC_DISABLE4 = 0x5B0;
static constexpr size_t APBDEV_PMC_SEC_DISABLE5 = 0x5B4;
static constexpr size_t APBDEV_PMC_SEC_DISABLE6 = 0x5B8;
static constexpr size_t APBDEV_PMC_SEC_DISABLE7 = 0x5BC;
static constexpr size_t APBDEV_PMC_SEC_DISABLE8 = 0x5C0;
static constexpr size_t APBDEV_PMC_SCRATCH188 = 0x810;
static constexpr size_t APBDEV_PMC_SCRATCH190 = 0x818;
static constexpr size_t APBDEV_PMC_SCRATCH200 = 0x840;
