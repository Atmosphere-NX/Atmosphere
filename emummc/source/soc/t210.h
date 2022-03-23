/*
* Copyright (c) 2018 naehrwert
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

#ifndef _T210_H_
#define _T210_H_

#include "../utils/types.h"

intptr_t QueryIoMapping(u64 addr, u64 size);

#define TMR_BASE 0x60005000
#define CLOCK_BASE 0x60006000
#define GPIO_BASE 0x6000D000
#define GPIO_1_BASE (GPIO_BASE)
#define GPIO_2_BASE (GPIO_BASE + 0x100)
#define GPIO_3_BASE (GPIO_BASE + 0x200)
#define GPIO_4_BASE (GPIO_BASE + 0x300)
#define GPIO_5_BASE (GPIO_BASE + 0x400)
#define GPIO_6_BASE (GPIO_BASE + 0x500)
#define GPIO_7_BASE (GPIO_BASE + 0x600)
#define GPIO_8_BASE (GPIO_BASE + 0x700)
#define APB_MISC_BASE 0x70000000
#define PINMUX_AUX_BASE 0x70003000
#define PWM_BASE 0x7000A000
#define RTC_BASE 0x7000E000
#define PMC_BASE 0x7000E400

#define _REG(base, off) *(vu32 *)(QueryIoMapping((u64)base, 0) + (off))
#define _REG_IO(base, off, size) *(vu32 *)(QueryIoMapping((u64)base, size) + (off))

#define RTC(off) _REG_IO(RTC_BASE, off, 0x4000)
#define TMR(off) _REG_IO(TMR_BASE, off, 0x3FF)
#define CLOCK(off) _REG_IO(CLOCK_BASE, off, 0x1000)

#define PMC(off) _REG_IO(PMC_BASE, off, 0x1000) // ??????????

#define APB_MISC(off) _REG_IO(APB_MISC_BASE, off, 0x4000)
#define PINMUX_AUX(off) _REG_IO(APB_MISC_BASE, off + (PINMUX_AUX_BASE - APB_MISC_BASE), 0x4000)

#define GPIO(off) _REG_IO(GPIO_BASE, off, 0x1000)
#define GPIO_1(off) _REG_IO(GPIO_BASE, off + (GPIO_1_BASE - GPIO_BASE), 0x1000)
#define GPIO_2(off) _REG_IO(GPIO_BASE, off + (GPIO_2_BASE - GPIO_BASE), 0x1000)
#define GPIO_3(off) _REG_IO(GPIO_BASE, off + (GPIO_3_BASE - GPIO_BASE), 0x1000)
#define GPIO_4(off) _REG_IO(GPIO_BASE, off + (GPIO_4_BASE - GPIO_BASE), 0x1000)
#define GPIO_5(off) _REG_IO(GPIO_BASE, off + (GPIO_5_BASE - GPIO_BASE), 0x1000)
#define GPIO_6(off) _REG_IO(GPIO_BASE, off + (GPIO_6_BASE - GPIO_BASE), 0x1000)
#define GPIO_7(off) _REG_IO(GPIO_BASE, off + (GPIO_7_BASE - GPIO_BASE), 0x1000)
#define GPIO_8(off) _REG_IO(GPIO_BASE, off + (GPIO_8_BASE - GPIO_BASE), 0x1000)

#define HOST1X(off) _REG(HOST1X_BASE, off)
#define BPMP_CACHE_CTRL(off) _REG(BPMP_CACHE_BASE, off)
#define DISPLAY_A(off) _REG(DISPLAY_A_BASE, off)
#define DSI(off) _REG(DSI_BASE, off)
#define VIC(off) _REG(VIC_BASE, off)
#define TSEC(off) _REG(TSEC_BASE, off)
#define SOR1(off) _REG(SOR1_BASE, off)
#define FLOW_CTLR(off) _REG(FLOW_CTLR_BASE, off)
#define SYSREG(off) _REG(SYSREG_BASE, off)
#define SB(off) _REG(SB_BASE, off)
#define EXCP_VEC(off) _REG(EXCP_VEC_BASE, off)
#define PWM(off) _REG(PWM_BASE, off)
#define SYSCTR0(off) _REG(SYSCTR0_BASE, off)
#define FUSE(off) _REG(FUSE_BASE, off)
#define KFUSE(off) _REG(KFUSE_BASE, off)
#define SE(off) _REG(SE_BASE, off)
#define MC(off) _REG(MC_BASE, off)
#define EMC(off) _REG(EMC_BASE, off)
#define MIPI_CAL(off) _REG(MIPI_CAL_BASE, off)
#define I2S(off) _REG(I2S_BASE, off)
#define CL_DVFS(off) _REG(CL_DVFS_BASE, off)
#define TEST_REG(off) _REG(0x0, off)

/*! Misc registers. */
#define APB_MISC_PP_STRAPPING_OPT_A 0x08
#define APB_MISC_PP_PINMUX_GLOBAL 0x40
#define APB_MISC_GP_LCD_BL_PWM_CFGPADCTRL 0xA34
#define APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL 0xA98
#define APB_MISC_GP_EMMC2_PAD_CFGPADCTRL 0xA9C
#define APB_MISC_GP_EMMC4_PAD_CFGPADCTRL 0xAB4
#define APB_MISC_GP_EMMC4_PAD_PUPD_CFGPADCTRL 0xABC
#define APB_MISC_GP_WIFI_EN_CFGPADCTRL 0xB64
#define APB_MISC_GP_WIFI_RST_CFGPADCTRL 0xB68

/*! System registers. */
#define AHB_ARBITRATION_XBAR_CTRL 0xE0
#define AHB_AHB_SPARE_REG 0x110

/*! RTC registers. */
#define APBDEV_RTC_SECONDS        0x8
#define APBDEV_RTC_SHADOW_SECONDS 0xC
#define APBDEV_RTC_MILLI_SECONDS  0x10

/*! TMR registers. */
#define TIMERUS_CNTR_1US          (0x10 + 0x0)
#define TIMERUS_USEC_CFG          (0x10 + 0x4)
#define TIMER_TMR9_TMR_PTV        0x80
#define  TIMER_EN     (1 << 31)
#define  TIMER_PER_EN (1 << 30)
#define TIMER_WDT4_CONFIG         (0x100 + 0x80)
#define  TIMER_SRC(TMR) (TMR & 0xF)
#define  TIMER_PER(PER) ((PER & 0xFF) << 4)
#define  TIMER_SYSRESET_EN (1 << 14)
#define  TIMER_PMCRESET_EN (1 << 15)
#define TIMER_WDT4_COMMAND        (0x108 + 0x80)
#define  TIMER_START_CNT   (1 << 0)
#define  TIMER_CNT_DISABLE (1 << 1)
#define TIMER_WDT4_UNLOCK_PATTERN (0x10C + 0x80)
#define  TIMER_MAGIC_PTRN 0xC45A

#endif
