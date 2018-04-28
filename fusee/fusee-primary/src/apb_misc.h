#ifndef __APB_MISC_H__
#define __APB_MISC_H__

/* FIXME: clean up */

#define MISC_BASE                               (0x70000000UL)
#define PINMUX_BASE                             (MISC_BASE + 0x3000)
#define PINMUX_AUX_GPIO_PZ1_0                   (*(volatile uint32_t *)(PINMUX_BASE + 0x280))

#define APB_MISC_GP_VGPIO_GPIO_MUX_SEL_0        MAKE_REG32(MISC_BASE + 0xb74)
#define APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL_0     MAKE_REG32(MISC_BASE + 0xa98)
#define APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0      MAKE_REG32(MISC_BASE + 0xab4)

#endif
