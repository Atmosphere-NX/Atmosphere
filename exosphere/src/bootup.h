#ifndef EXOSPHERE_BOOTUP_H
#define EXOSPHERE_BOOTUP_H

#include <stdint.h>

/* APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0 slaves */
enum APB_SSER0 {
    MISC_REGS  = 1 << 1, /* PP, SC1x pads and GP registers */
    SATA_AUX   = 1 << 2,
    PINMUX_AUX = 1 << 3,
    APE        = 1 << 4,

    DTV        = 1 << 6,

    PWM        = 1 << 8, /* PWFM */
    QSPI       = 1 << 9,
    CSITE      = 1 << 10, /* Core Site */
    RTC        = 1 << 11,

    PMC        = 1 << 13,
    SE         = 1 << 14, /* Security Engine */
    FUSE       = 1 << 15,
    KFUSE      = 1 << 16,

    UNUSED     = 1 << 18, /* reserved, unused but listed as accessible */

    SATA       = 1 << 20,
    HDA        = 1 << 21,
    LA         = 1 << 22,
    ATOMICS    = 1 << 23,
    CEC        = 1 << 24,

    STM        = 1 << 29
};

/* APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0 slaves */
enum APB_SSER1 {
    MC0    = 1 << 4,
    EMC0   = 1 << 5,

    MC1    = 1 << 8,
    EMC1   = 1 << 9,
    MCB    = 1 << 10,
    EMBC   = 1 << 11,
    UART_A = 1 << 12,
    UART_B = 1 << 13,
    UART_C = 1 << 14,
    UART_D = 1 << 15,

    SPI1   = 1 << 20,
    SPI2   = 1 << 21,
    SPI3   = 1 << 22,
    SPI4   = 1 << 23,
    SPI5   = 1 << 24,
    SPI6   = 1 << 25,
    I2C1   = 1 << 26,
    I2C2   = 1 << 27,
    I2C3   = 1 << 28,
    I2C4   = 1 << 29,
    DVC    = 1 << 30,
    I2C5   = 1 << 30,
    I2C6   = 1 << 31 /* this will show as negative because of the 32bit sign bit being set */
};

/* APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG2_0 slaves */
enum APB_SSER2 {
    SDMMC1      = 1 << 0,
    SDMMC2      = 1 << 1,
    SDMMC3      = 1 << 2,
    SDMMC4      = 1 << 3,

    MIPIBIF     = 1 << 7, /* reserved */
    DDS         = 1 << 8,
    DP2         = 1 << 9,
    SOC_THERM   = 1 << 10,
    APB2JTAG    = 1 << 11,
    XUSB_HOST   = 1 << 12,
    XUSB_DEV    = 1 << 13,
    XUSB_PADCTL = 1 << 14,
    MIPI_CAL    = 1 << 15,
    DVFS        = 1 << 16
};

void bootup_misc_mmio(void);

void setup_4x_mmio(void);

void setup_current_core_state(void);

void identity_unmap_iram_cd_tzram(void);

void secure_additional_devices(void);

void set_extabt_serror_taken_to_el3(bool taken_to_el3);

#endif
