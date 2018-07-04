#ifndef FUSEE_APB_MISC_H
#define FUSEE_APB_MISC_H

#define SDMMC1_PAD_CAL_DRVUP_SHIFT              (20)
#define SDMMC1_PAD_CAL_DRVDN_SHIFT              (12)
#define SDMMC1_PAD_CAL_DRVUP_MASK               (0x7Fu << SDMMC1_PAD_CAL_DRVUP_SHIFT)
#define SDMMC1_PAD_CAL_DRVDN_MASK               (0x7Fu << SDMMC1_PAD_CAL_DRVDN_SHIFT)

#define CFG2TMC_EMMC4_PAD_DRVUP_COMP_SHIFT      (8)
#define CFG2TMC_EMMC4_PAD_DRVDN_COMP_SHIFT      (2)
#define CFG2TMC_EMMC4_PAD_DRVUP_COMP_MASK       (0x3Fu << CFG2TMC_EMMC4_PAD_DRVUP_COMP_SHIFT)
#define CFG2TMC_EMMC4_PAD_DRVDN_COMP_MASK       (0x3Fu << CFG2TMC_EMMC4_PAD_DRVDN_COMP_SHIFT)

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
    uint32_t reserved0[0x31];
    uint32_t sdmmc1_clk_lpbk_control;       /* 0x8D4 */
    uint32_t sdmmc3_clk_lpbk_control;       /* 0x8D8 */
    uint32_t emmc2_pad_cfg_control;         /* 0x8DC */
    uint32_t emmc4_pad_cfg_control;         /* 0x8E0 */
    uint32_t _todo0[0x6E];
    uint32_t sdmmc1_pad_cfgpadctrl;         /* 0xA98 */
    uint32_t emmc2_pad_cfgpadctrl;          /* 0xA9C */
    uint32_t emmc2_pad_drv_type_cfgpadctrl; /* 0xAA0 */
    uint32_t emmc2_pad_pupd_cfgpadctrl;     /* 0xAA4 */
    uint32_t _todo1[0x03];
    uint32_t sdmmc3_pad_cfgpadctrl;         /* 0xAB0 */
    uint32_t emmc4_pad_cfgpadctrl;          /* 0xAB4 */
    uint32_t emmc4_pad_drv_type_cfgpadctrl; /* 0xAB8 */
    uint32_t emmc4_pad_pupd_cfgpadctrl;     /* 0xABC */
    uint32_t _todo2[0x2E];
    uint32_t vgpio_gpio_mux_sel;            /* 0xB74 */
    uint32_t qspi_sck_lpbk_control;         /* 0xB78 */
} tegra_padctl_t;

static inline volatile tegra_padctl_t *padctl_get_regs(void)
{
    return (volatile tegra_padctl_t *)0x70000810;
}

#endif
