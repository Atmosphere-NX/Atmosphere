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

static constexpr size_t CLK_RST_CONTROLLER_RST_SOURCE = 0x0;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEVICES_L = 0x4;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEVICES_H = 0x8;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEVICES_U = 0xC;
static constexpr size_t CLK_RST_CONTROLLER_CLK_OUT_ENB_L = 0x10;
static constexpr size_t CLK_RST_CONTROLLER_CLK_OUT_ENB_H = 0x14;
static constexpr size_t CLK_RST_CONTROLLER_CLK_OUT_ENB_U = 0x18;
static constexpr size_t CLK_RST_CONTROLLER_CCLK_BURST_POLICY = 0x20;
static constexpr size_t CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER = 0x24;
static constexpr size_t CLK_RST_CONTROLLER_SCLK_BURST_POLICY = 0x28;
static constexpr size_t CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER = 0x2C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SYSTEM_RATE = 0x30;
static constexpr size_t CLK_RST_CONTROLLER_MISC_CLK_ENB = 0x48;
static constexpr size_t CLK_RST_CONTROLLER_OSC_CTRL = 0x50;
static constexpr size_t CLK_RST_CONTROLLER_PLLC_BASE = 0x80;
static constexpr size_t CLK_RST_CONTROLLER_PLLC_MISC = 0x88;
static constexpr size_t CLK_RST_CONTROLLER_PLLM_BASE = 0x90;
static constexpr size_t CLK_RST_CONTROLLER_PLLM_MISC1 = 0x98;
static constexpr size_t CLK_RST_CONTROLLER_PLLM_MISC2 = 0x9C;
static constexpr size_t CLK_RST_CONTROLLER_PLLP_BASE = 0xA0;
static constexpr size_t CLK_RST_CONTROLLER_PLLD_BASE = 0xD0;
static constexpr size_t CLK_RST_CONTROLLER_PLLD_MISC1 = 0xD8;
static constexpr size_t CLK_RST_CONTROLLER_PLLD_MISC = 0xDC;
static constexpr size_t CLK_RST_CONTROLLER_PLLX_BASE = 0xE0;
static constexpr size_t CLK_RST_CONTROLLER_PLLX_MISC = 0xE4;
static constexpr size_t CLK_RST_CONTROLLER_PLLE_BASE = 0xE8;
static constexpr size_t CLK_RST_CONTROLLER_PLLE_MISC = 0xEC;
static constexpr size_t CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA = 0xF8;
static constexpr size_t CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB = 0xFC;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_PWM = 0x110;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_I2C1 = 0x124;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_I2C5 = 0x128;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_DISP1 = 0x138;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_VI = 0x148;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1 = 0x150;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2 = 0x154;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4 = 0x164;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_UARTA = 0x178;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_UARTB = 0x17C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X = 0x180;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_UARTC = 0x1A0;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_I2C3 = 0x1B8;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3 = 0x1BC;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_CSITE = 0x1D4;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_EMC = 0x19C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_TSEC = 0x1F4;
static constexpr size_t CLK_RST_CONTROLLER_CLK_OUT_ENB_X = 0x280;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_X_SET = 0x284;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_X_CLR = 0x288;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEVICES_X = 0x28C;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_X_SET = 0x290;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_X_CLR = 0x294;
static constexpr size_t CLK_RST_CONTROLLER_CLK_OUT_ENB_Y = 0x298;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_Y_SET = 0x29C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_Y_CLR = 0x2A0;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEVICES_Y = 0x2A4;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_Y_SET = 0x2A8;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_Y_CLR = 0x2AC;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_L_SET = 0x300;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_L_CLR = 0x304;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_H_SET = 0x308;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_H_CLR = 0x30C;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_U_SET = 0x310;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEV_U_CLR = 0x314;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_L_SET = 0x320;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_L_CLR = 0x324;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_H_SET = 0x328;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_H_CLR = 0x32C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_U_SET = 0x330;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_U_CLR = 0x334;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEVICES_V = 0x358;
static constexpr size_t CLK_RST_CONTROLLER_RST_DEVICES_W = 0x35C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_OUT_ENB_V = 0x360;
static constexpr size_t CLK_RST_CONTROLLER_CLK_OUT_ENB_W = 0x364;
static constexpr size_t CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2 = 0x388;
static constexpr size_t CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC = 0x3A0;
static constexpr size_t CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD = 0x3A4;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT = 0x3B4;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_SOR1 = 0x410;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_SE = 0x42C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_V_SET = 0x440;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_W_SET = 0x448;
static constexpr size_t CLK_RST_CONTROLLER_CLK_ENB_W_CLR = 0x44C;
static constexpr size_t CLK_RST_CONTROLLER_RST_CPUG_CMPLX_SET = 0x450;
static constexpr size_t CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR = 0x454;
static constexpr size_t CLK_RST_CONTROLLER_UTMIP_PLL_CFG2 = 0x488;
static constexpr size_t CLK_RST_CONTROLLER_PLLE_AUX = 0x48C;
static constexpr size_t CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S0 = 0x4A0;
static constexpr size_t CLK_RST_CONTROLLER_PLLX_MISC_3 = 0x518;
static constexpr size_t CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE = 0x554;
static constexpr size_t CLK_RST_CONTROLLER_SPARE_REG0 = 0x55C;
static constexpr size_t CLK_RST_CONTROLLER_PLLMB_BASE = 0x5E8;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP = 0x620;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL = 0x664;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL = 0x66C;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM = 0x694;
static constexpr size_t CLK_RST_CONTROLLER_CLK_SOURCE_NVENC = 0x6A0;
static constexpr size_t CLK_RST_CONTROLLER_SE_SUPER_CLK_DIVIDER = 0x704;
