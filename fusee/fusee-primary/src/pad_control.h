/**
 * Fusée pad control code
 *  ~ktemkin
 */

#ifndef __FUSEE_PADCTL_H__
#define __FUSEE_PADCTL_H__

#include "utils.h"

/**
 * Registers in the Misc Pad control region
 */
struct PACKED tegra_padctl {
    /* TODO: support registers before? */
    uint32_t sdmmc1_control;
    uint32_t sdmmc3_control;
    uint32_t sdmmc2_control;
    uint32_t sdmmc4_control;

    /* TODO: support registers after? */
    uint8_t _todo[656];

    uint32_t vgpio_gpio_mux_sel;
};

/** 
 * Masks for Pad Control registers
 */
enum tegra_padctl_masks {

    /* SDMMC1 */
    PADCTL_SDMMC1_DEEP_LOOPBACK = (1 << 0),

    /* SDMMC3 */
    PADCTL_SDMMC3_DEEP_LOOPBACK = (1 << 0),

    /* SDMMC2 */
    PADCTL_SDMMC2_ENABLE_DATA_IN = (0xFF << 8),
    PADCTL_SDMMC2_ENABLE_CLK_IN  = (0x3  << 4),
    PADCTL_SDMMC2_DEEP_LOOPBACK  = (1    << 0),

    /* SDMMC4 */
    PADCTL_SDMMC4_ENABLE_DATA_IN = (0xFF << 8),
    PADCTL_SDMMC4_ENABLE_CLK_IN  = (0x3  << 4),
    PADCTL_SDMMC4_DEEP_LOOPBACK  = (1    << 0),

    /* VGPIO/GPIO */
    PADCTL_SDMMC1_CD_SOURCE = (1 << 0),
    PADCTL_SDMMC1_WP_SOURCE = (1 << 1),
    PADCTL_SDMMC3_CD_SOURCE = (1 << 2),
    PADCTL_SDMMC3_WP_SOURCE = (1 << 3),


};


/**
 * Utility function that grabs the Tegra PADCTL registers.
 */
static inline struct tegra_padctl *padctl_get_regs(void)
{
    return (struct tegra_padctl *)0x700008d4;
}


#endif
