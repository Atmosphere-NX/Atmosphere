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

static constexpr size_t APB_MISC_GP_SDMMC1_CLK_LPBK_CONTROL = 0x8D4;
static constexpr size_t APB_MISC_GP_SDMMC3_CLK_LPBK_CONTROL = 0x8D8;
static constexpr size_t APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL   = 0xA98;
static constexpr size_t APB_MISC_GP_VGPIO_GPIO_MUX_SEL      = 0xB74;

static constexpr size_t PINMUX_AUX_SDMMC1_CLK  = 0x00;
static constexpr size_t PINMUX_AUX_SDMMC1_CMD  = 0x04;
static constexpr size_t PINMUX_AUX_SDMMC1_DAT3 = 0x08;
static constexpr size_t PINMUX_AUX_SDMMC1_DAT2 = 0x0C;
static constexpr size_t PINMUX_AUX_SDMMC1_DAT1 = 0x10;
static constexpr size_t PINMUX_AUX_SDMMC1_DAT0 = 0x14;
static constexpr size_t PINMUX_AUX_SDMMC3_CLK  = 0x1C;
static constexpr size_t PINMUX_AUX_SDMMC3_CMD  = 0x20;
static constexpr size_t PINMUX_AUX_SDMMC3_DAT0 = 0x24;
static constexpr size_t PINMUX_AUX_SDMMC3_DAT1 = 0x28;
static constexpr size_t PINMUX_AUX_SDMMC3_DAT2 = 0x2C;
static constexpr size_t PINMUX_AUX_SDMMC3_DAT3 = 0x30;
static constexpr size_t PINMUX_AUX_DMIC3_CLK   = 0xB4;
static constexpr size_t PINMUX_AUX_UART2_TX    = 0xF4;
static constexpr size_t PINMUX_AUX_UART3_TX    = 0x104;
static constexpr size_t PINMUX_AUX_WIFI_EN     = 0x1B4;
static constexpr size_t PINMUX_AUX_WIFI_RST    = 0x1B8;
static constexpr size_t PINMUX_AUX_NFC_EN      = 0x1D0;
static constexpr size_t PINMUX_AUX_NFC_INT     = 0x1D4;
static constexpr size_t PINMUX_AUX_LCD_BL_PWM  = 0x1FC;
static constexpr size_t PINMUX_AUX_LCD_BL_EN   = 0x200;
static constexpr size_t PINMUX_AUX_LCD_RST     = 0x204;
static constexpr size_t PINMUX_AUX_GPIO_PE6    = 0x248;
static constexpr size_t PINMUX_AUX_GPIO_PH6    = 0x250;
static constexpr size_t PINMUX_AUX_GPIO_PZ1    = 0x280;

static constexpr u32 PINMUX_FUNC_MASK    = (3 << 0);

static constexpr u32 PINMUX_PULL_MASK    = (3 << 2);
static constexpr u32 PINMUX_PULL_NONE    = (0 << 2);
static constexpr u32 PINMUX_PULL_DOWN    = (1 << 2);
static constexpr u32 PINMUX_PULL_UP      = (2 << 2);

static constexpr u32 PINMUX_TRISTATE     = (1 << 4);
static constexpr u32 PINMUX_PARKED       = (1 << 5);
static constexpr u32 PINMUX_INPUT_ENABLE = (1 << 6);
static constexpr u32 PINMUX_LOCK         = (1 << 7);
static constexpr u32 PINMUX_LPDR         = (1 << 8);
static constexpr u32 PINMUX_HSM          = (1 << 9);

static constexpr u32 PINMUX_IO_HV        = (1 << 10);
static constexpr u32 PINMUX_OPEN_DRAIN   = (1 << 11);
static constexpr u32 PINMUX_SCHMT        = (1 << 12);

static constexpr u32 PINMUX_DRIVE_1X     = (0 << 13) ;
static constexpr u32 PINMUX_DRIVE_2X     = (1 << 13);
static constexpr u32 PINMUX_DRIVE_3X     = (2 << 13);
static constexpr u32 PINMUX_DRIVE_4X     = (3 << 13);

