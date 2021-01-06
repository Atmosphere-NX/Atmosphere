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
 
#ifndef FUSEE_APB_MISC_H
#define FUSEE_APB_MISC_H

#include <stdint.h>

#define APB_MISC_BASE 0x70000000
#define APB_PADCTL_BASE 0x70000810
#define MAKE_APB_MISC_REG(n) MAKE_REG32(APB_MISC_BASE + n)
#define MAKE_APB_PADCTL_REG(n) MAKE_REG32(APB_PADCTL_BASE + n)

#define APB_MISC_PP_PINMUX_GLOBAL_0 MAKE_APB_MISC_REG(0x40)
#define APB_MISC_GP_DSI_PAD_CONTROL_0 MAKE_APB_MISC_REG(0xAC0)
#define APB_MISC_GP_WIFI_EN_CFGPADCTRL_0 MAKE_APB_MISC_REG(0xB64)
#define APB_MISC_GP_WIFI_RST_CFGPADCTRL_0 MAKE_APB_MISC_REG(0xB68)

#define SDMMC1_PAD_CAL_DRVUP_SHIFT              (20)
#define SDMMC1_PAD_CAL_DRVDN_SHIFT              (12)
#define SDMMC1_CLK_CFG_CAL_DRVDN_SLWR_SHIFT     (28)
#define SDMMC1_CLK_CFG_CAL_DRVDN_SLWF_SHIFT     (30)
#define SDMMC1_PAD_CAL_DRVUP_MASK               (0x7Fu << SDMMC1_PAD_CAL_DRVUP_SHIFT)
#define SDMMC1_PAD_CAL_DRVDN_MASK               (0x7Fu << SDMMC1_PAD_CAL_DRVDN_SHIFT)
#define SDMMC1_CLK_CFG_CAL_DRVDN_SLWR_MASK      (0x03u << SDMMC1_CLK_CFG_CAL_DRVDN_SLWR_SHIFT)
#define SDMMC1_CLK_CFG_CAL_DRVDN_SLWF_MASK      (0x03u << SDMMC1_CLK_CFG_CAL_DRVDN_SLWF_SHIFT)

#define EMMC2_PAD_DRVUP_COMP_SHIFT      (8)
#define EMMC2_PAD_DRVDN_COMP_SHIFT      (2)
#define EMMC2_PAD_DRVUP_COMP_MASK       (0x3Fu << EMMC2_PAD_DRVUP_COMP_SHIFT)
#define EMMC2_PAD_DRVDN_COMP_MASK       (0x3Fu << EMMC2_PAD_DRVDN_COMP_SHIFT)

#define SDMMC2_PAD_CAL_DRVUP_SHIFT      (20)
#define SDMMC2_PAD_CAL_DRVDN_SHIFT      (12)
#define SDMMC2_PAD_CAL_DRVUP_MASK       (0x7Fu << SDMMC2_PAD_CAL_DRVUP_SHIFT)
#define SDMMC2_PAD_CAL_DRVDN_MASK       (0x7Fu << SDMMC2_PAD_CAL_DRVDN_SHIFT)

#define EMMC4_PAD_DRVUP_COMP_SHIFT      (8)
#define EMMC4_PAD_DRVDN_COMP_SHIFT      (2)
#define EMMC4_PAD_DRVUP_COMP_MASK       (0x3Fu << EMMC4_PAD_DRVUP_COMP_SHIFT)
#define EMMC4_PAD_DRVDN_COMP_MASK       (0x3Fu << EMMC4_PAD_DRVDN_COMP_SHIFT)

#define PADCTL_SDMMC1_DEEP_LOOPBACK     (1 << 0)
#define PADCTL_SDMMC3_DEEP_LOOPBACK     (1 << 0)
#define PADCTL_SDMMC2_ENABLE_DATA_IN    (0xFF << 8)
#define PADCTL_SDMMC2_ENABLE_CLK_IN     (0x3 << 4)
#define PADCTL_SDMMC2_DEEP_LOOPBACK     (1 << 0)
#define PADCTL_SDMMC4_ENABLE_DATA_IN    (0xFF << 8)
#define PADCTL_SDMMC4_ENABLE_CLK_IN     (0x3 << 4)
#define PADCTL_SDMMC4_DEEP_LOOPBACK     (1 << 0)
#define PADCTL_SDMMC1_CD_SOURCE         (1 << 0)
#define PADCTL_SDMMC1_WP_SOURCE         (1 << 1)
#define PADCTL_SDMMC3_CD_SOURCE         (1 << 2)
#define PADCTL_SDMMC3_WP_SOURCE         (1 << 3)

typedef struct {
    uint32_t asdbgreg;                      /* 0x810 */
    uint32_t _0x814[0x31];
    uint32_t sdmmc1_clk_lpbk_control;       /* 0x8D4 */
    uint32_t sdmmc3_clk_lpbk_control;       /* 0x8D8 */
    uint32_t emmc2_pad_cfg_control;         /* 0x8DC */
    uint32_t emmc4_pad_cfg_control;         /* 0x8E0 */
    uint32_t _0x8E4[0x6E];
    uint32_t sdmmc1_pad_cfgpadctrl;         /* 0xA98 */
    uint32_t emmc2_pad_cfgpadctrl;          /* 0xA9C */
    uint32_t emmc2_pad_drv_type_cfgpadctrl; /* 0xAA0 */
    uint32_t emmc2_pad_pupd_cfgpadctrl;     /* 0xAA4 */
    uint32_t _0xAA8[0x03];
    uint32_t sdmmc3_pad_cfgpadctrl;         /* 0xAB0 */
    uint32_t emmc4_pad_cfgpadctrl;          /* 0xAB4 */
    uint32_t emmc4_pad_drv_type_cfgpadctrl; /* 0xAB8 */
    uint32_t emmc4_pad_pupd_cfgpadctrl;     /* 0xABC */
    uint32_t _0xAC0[0x2E];
    uint32_t vgpio_gpio_mux_sel;            /* 0xB74 */
    uint32_t qspi_sck_lpbk_control;         /* 0xB78 */
} tegra_padctl_t;

static inline volatile tegra_padctl_t *padctl_get_regs(void)
{
    return (volatile tegra_padctl_t *)APB_PADCTL_BASE;
}

#endif
