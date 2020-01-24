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
 
#ifndef EXOSPHERE_BOOTUP_H
#define EXOSPHERE_BOOTUP_H

#include <stdint.h>

/* 21.1.7 AP Control Registers */
/* 21.1.7.1 APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0 slaves */
typedef enum {
    APB_SSER0_MISC_REGS  = 1 << 1, /* PP, SC1x pads and GP registers */
    APB_SSER0_SATA_AUX   = 1 << 2,
    APB_SSER0_PINMUX_AUX = 1 << 3,
    APB_SSER0_APE        = 1 << 4,

    APB_SSER0_DTV        = 1 << 6,

    APB_SSER0_PWM        = 1 << 8, /* PWFM */
    APB_SSER0_QSPI       = 1 << 9,
    APB_SSER0_CSITE      = 1 << 10, /* Core Site */
    APB_SSER0_RTC        = 1 << 11,

    APB_SSER0_PMC        = 1 << 13,
    APB_SSER0_SE         = 1 << 14, /* Security Engine */
    APB_SSER0_FUSE       = 1 << 15,
    APB_SSER0_KFUSE      = 1 << 16,

    APB_SSER0_UNUSED     = 1 << 18, /* reserved, unused but listed as accessible */

    APB_SSER0_SATA       = 1 << 20,
    APB_SSER0_HDA        = 1 << 21,
    APB_SSER0_LA         = 1 << 22,
    APB_SSER0_ATOMICS    = 1 << 23,
    APB_SSER0_CEC        = 1 << 24,

    STM        = 1 << 29
} APB_SSER0;

/* 21.1.7.2 APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0 slaves */
typedef enum {
    APB_SSER1_MC0    = 1 << 4,
    APB_SSER1_EMC0   = 1 << 5,

    APB_SSER1_MC1    = 1 << 8,
    APB_SSER1_EMC1   = 1 << 9,
    APB_SSER1_MCB    = 1 << 10,
    APB_SSER1_EMBC   = 1 << 11,
    APB_SSER1_UART_A = 1 << 12,
    APB_SSER1_UART_B = 1 << 13,
    APB_SSER1_UART_C = 1 << 14,
    APB_SSER1_UART_D = 1 << 15,

    APB_SSER1_SPI1   = 1 << 20,
    APB_SSER1_SPI2   = 1 << 21,
    APB_SSER1_SPI3   = 1 << 22,
    APB_SSER1_SPI4   = 1 << 23,
    APB_SSER1_SPI5   = 1 << 24,
    APB_SSER1_SPI6   = 1 << 25,
    APB_SSER1_I2C1   = 1 << 26,
    APB_SSER1_I2C2   = 1 << 27,
    APB_SSER1_I2C3   = 1 << 28,
    APB_SSER1_I2C4   = 1 << 29,
    APB_SSER1_DVC    = 1 << 30,
    APB_SSER1_I2C5   = 1 << 30,
    APB_SSER1_I2C6   = 1 << 31 /* this will show as negative because of the 32bit sign bit being set */
} APB_SSER1;

/* 21.1.7.3 APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG2_0 slaves */
typedef enum {
    APB_SSER2_SDMMC1      = 1 << 0,
    APB_SSER2_SDMMC2      = 1 << 1,
    APB_SSER2_SDMMC3      = 1 << 2,
    APB_SSER2_SDMMC4      = 1 << 3,

    APB_SSER2_MIPIBIF     = 1 << 7, /* reserved */
    APB_SSER2_DDS         = 1 << 8,
    APB_SSER2_DP2         = 1 << 9,
    APB_SSER2_SOC_THERM   = 1 << 10,
    APB_SSER2_APB2JTAG    = 1 << 11,
    APB_SSER2_XUSB_HOST   = 1 << 12,
    APB_SSER2_XUSB_DEV    = 1 << 13,
    APB_SSER2_XUSB_PADCTL = 1 << 14,
    APB_SSER2_MIPI_CAL    = 1 << 15,
    APB_SSER2_DVFS        = 1 << 16
} APB_SSER2;

void bootup_misc_mmio(void);

void setup_4x_mmio(void);

void setup_dram_magic_numbers(void);

void setup_current_core_state(void);

void identity_unmap_iram_cd_tzram(void);

void secure_additional_devices(void);

void set_extabt_serror_taken_to_el3(bool taken_to_el3);

#endif
