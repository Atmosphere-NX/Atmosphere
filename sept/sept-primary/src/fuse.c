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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <vapours/ams_version.h>

#include "car.h"
#include "fuse.h"
#include "pmc.h"
#include "timers.h"

/* Initialize the fuse driver */
void fuse_init(void) {
    /* Make all fuse registers visible, disable the private key and disable programming. */
    clkrst_enable_fuse_regs(true);
    /* fuse_disable_private_key(); */
    /* fuse_disable_programming(); */
}

/* Disable access to the private key and set the TZ sticky bit. */
void fuse_disable_private_key(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse->FUSE_PRIVATEKEYDISABLE = 0x10;
}

/* Disable all fuse programming. */
void fuse_disable_programming(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse->FUSE_DISABLEREGPROGRAM = 1;
}

/* Enable power to the fuse hardware array. */
void fuse_enable_power(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    pmc->fuse_control &= ~(0x200);      /* Clear PMC_FUSE_CTRL_PS18_LATCH_CLEAR. */
    mdelay(1);
    pmc->fuse_control |= 0x100;         /* Set PMC_FUSE_CTRL_PS18_LATCH_SET. */
    mdelay(1);
}

/* Disable power to the fuse hardware array. */
void fuse_disable_power(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    pmc->fuse_control &= ~(0x100);      /* Clear PMC_FUSE_CTRL_PS18_LATCH_SET. */
    mdelay(1);
    pmc->fuse_control |= 0x200;         /* Set PMC_FUSE_CTRL_PS18_LATCH_CLEAR. */
    mdelay(1);
}

/* Wait for the fuse driver to go idle. */
static void fuse_wait_idle(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    uint32_t ctrl_val = 0;

    /* Wait for STATE_IDLE */
    while ((ctrl_val & (0xF0000)) != 0x40000) {
        ctrl_val = fuse->FUSE_FUSECTRL;
    }
}

/* Read a fuse from the hardware array. */
uint32_t fuse_hw_read(uint32_t addr) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();

    /* Wait for idle state. */
    fuse_wait_idle();

    /* Program the target address. */
    fuse->FUSE_FUSEADDR = addr;

    /* Enable read operation in control register. */
    uint32_t ctrl_val = fuse->FUSE_FUSECTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x1;    /* Set READ command. */
    fuse->FUSE_FUSECTRL = ctrl_val;

    /* Wait for idle state. */
    fuse_wait_idle();

    return fuse->FUSE_FUSERDATA;
}

/* Write a fuse in the hardware array. */
void fuse_hw_write(uint32_t value, uint32_t addr) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();

    /* Wait for idle state. */
    fuse_wait_idle();

    /* Program the target address and value. */
    fuse->FUSE_FUSEADDR = addr;
    fuse->FUSE_FUSEWDATA = value;

    /* Enable write operation in control register. */
    uint32_t ctrl_val = fuse->FUSE_FUSECTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x2;    /* Set WRITE command. */
    fuse->FUSE_FUSECTRL = ctrl_val;

    /* Wait for idle state. */
    fuse_wait_idle();
}

/* Sense the fuse hardware array into the fuse cache. */
void fuse_hw_sense(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();

    /* Wait for idle state. */
    fuse_wait_idle();

    /* Enable sense operation in control register */
    uint32_t ctrl_val = fuse->FUSE_FUSECTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x3;    /* Set SENSE_CTRL command */
    fuse->FUSE_FUSECTRL = ctrl_val;

    /* Wait for idle state. */
    fuse_wait_idle();
}

/* Read the SKU info register. */
uint32_t fuse_get_sku_info(void) {
    volatile tegra_fuse_chip_common_t *fuse_chip = fuse_chip_common_get_regs();
    return fuse_chip->FUSE_SKU_INFO;
}

/* Read the bootrom patch version. */
uint32_t fuse_get_bootrom_patch_version(void) {
    volatile tegra_fuse_chip_common_t *fuse_chip = fuse_chip_common_get_regs();
    return fuse_chip->FUSE_SOC_SPEEDO_1_CALIB;
}

/* Read a spare bit register. */
uint32_t fuse_get_spare_bit(uint32_t index) {
    uint32_t soc_type = fuse_get_soc_type();
    if (soc_type == 0) {
        if (index < 32) {
            volatile tegra_fuse_chip_erista_t *fuse_chip = fuse_chip_erista_get_regs();
            return fuse_chip->FUSE_SPARE_BIT[index];
        }
    } else if (soc_type == 1) {
        if (index < 30) {
            volatile tegra_fuse_chip_mariko_t *fuse_chip = fuse_chip_mariko_get_regs();
            return fuse_chip->FUSE_SPARE_BIT[index];
        }
    }
    return 0;
}

/* Read a reserved ODM register. */
uint32_t fuse_get_reserved_odm(uint32_t index) {
    if (index < 8) {
        volatile tegra_fuse_chip_common_t *fuse_chip = fuse_chip_common_get_regs();
        return fuse_chip->FUSE_RESERVED_ODM0[index];
    } else {
        uint32_t soc_type = fuse_get_soc_type();
        if (soc_type == 1) {
            volatile tegra_fuse_chip_mariko_t *fuse_chip = fuse_chip_mariko_get_regs();
            if (index < 22) {
                return fuse_chip->FUSE_RESERVED_ODM8[index - 8];
            } else if (index < 25) {
                return fuse_chip->FUSE_RESERVED_ODM22[index - 22];
            } else if (index < 26) {
                return fuse_chip->FUSE_RESERVED_ODM25;
            } else if (index < 29) {
                return fuse_chip->FUSE_RESERVED_ODM26[index - 26];
            } else if (index < 30) {
                return fuse_chip->FUSE_RESERVED_ODM29;
            }
        }
    }
    return 0;
}

/* Get the DramId. */
uint32_t fuse_get_dram_id(void) {
    return ((fuse_get_reserved_odm(4) >> 3) & 0x1F);
}

/* Derive the DeviceId. */
uint64_t fuse_get_device_id(void) {
    volatile tegra_fuse_chip_common_t *fuse_chip = fuse_chip_common_get_regs();

    uint64_t device_id = 0;
    uint64_t y_coord = fuse_chip->FUSE_OPT_Y_COORDINATE & 0x1FF;
    uint64_t x_coord = fuse_chip->FUSE_OPT_X_COORDINATE & 0x1FF;
    uint64_t wafer_id = fuse_chip->FUSE_OPT_WAFER_ID & 0x3F;
    uint32_t lot_code = fuse_chip->FUSE_OPT_LOT_CODE_0;
    uint64_t fab_code = fuse_chip->FUSE_OPT_FAB_CODE & 0x3F;

    uint64_t derived_lot_code = 0;
    for (unsigned int i = 0; i < 5; i++) {
        derived_lot_code = (derived_lot_code * 0x24) + ((lot_code >> (24 - 6*i)) & 0x3F);
    }
    derived_lot_code &= 0x03FFFFFF;

    device_id |= y_coord << 0;
    device_id |= x_coord << 9;
    device_id |= wafer_id << 18;
    device_id |= derived_lot_code << 24;
    device_id |= fab_code << 50;

    return device_id;
}

/* Derive the HardwareType with firmware specific checks. */
uint32_t fuse_get_hardware_type_with_firmware_check(uint32_t target_firmware) {
    uint32_t fuse_reserved_odm4 = fuse_get_reserved_odm(4);
    uint32_t hardware_type = (((fuse_reserved_odm4 >> 7) & 2) | ((fuse_reserved_odm4 >> 2) & 1));

    if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_4_0_0) {
        volatile tegra_fuse_chip_common_t *fuse_chip = fuse_chip_common_get_regs();
        uint32_t fuse_spare_bit9 = (fuse_chip->FUSE_SPARE_BIT[9] & 1);

        switch (hardware_type) {
            case 0x00: return (fuse_spare_bit9 == 0) ? 0 : 3;
            case 0x01: return 0;        /* HardwareType_Icosa */
            case 0x02: return 1;        /* HardwareType_Copper */
            default:   return 3;        /* HardwareType_Undefined */
        }
    } else {
        hardware_type |= ((fuse_reserved_odm4 >> 14) & 0x3C);

        if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_7_0_0) {
            switch (hardware_type) {
                case 0x01: return 0;        /* HardwareType_Icosa */
                case 0x02: return 1;        /* HardwareType_Copper */
                case 0x04: return 3;        /* HardwareType_Iowa */
                default:   return 4;        /* HardwareType_Undefined */
            }
        } else {
            if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_10_0_0) {
                switch (hardware_type) {
                    case 0x01: return 0;        /* HardwareType_Icosa */
                    case 0x02: return 4;        /* HardwareType_Calcio */
                    case 0x04: return 3;        /* HardwareType_Iowa */
                    case 0x08: return 2;        /* HardwareType_Hoag */
                    default:   return 0xF;      /* HardwareType_Undefined */
                }
            } else {
                switch (hardware_type) {
                    case 0x01: return 0;        /* HardwareType_Icosa */
                    case 0x02: return 4;        /* HardwareType_Calcio */
                    case 0x04: return 3;        /* HardwareType_Iowa */
                    case 0x08: return 2;        /* HardwareType_Hoag */
                    case 0x10: return 5;        /* HardwareType_Five */
                    default:   return 0xF;      /* HardwareType_Undefined */
                }
            }
        }
    }
}

/* Derive the HardwareType. */
uint32_t fuse_get_hardware_type(void) {
    return fuse_get_hardware_type_with_firmware_check(ATMOSPHERE_TARGET_FIRMWARE_CURRENT);
}

/* Derive the HardwareState. */
uint32_t fuse_get_hardware_state(void) {
    uint32_t fuse_reserved_odm4 = fuse_get_reserved_odm(4);
    uint32_t hardware_state = (((fuse_reserved_odm4 >> 7) & 4) | (fuse_reserved_odm4 & 3));

    switch (hardware_state) {
        case 0x03: return 0;        /* HardwareState_Development */
        case 0x04: return 1;        /* HardwareState_Production */
        default:   return 2;        /* HardwareState_Undefined */
    }
}

/* Derive the 16-byte HardwareInfo and copy to output buffer. */
void fuse_get_hardware_info(void *dst) {
    volatile tegra_fuse_chip_common_t *fuse_chip = fuse_chip_common_get_regs();
    uint32_t hw_info[0x4];

    uint32_t ops_reserved = fuse_chip->FUSE_OPT_OPS_RESERVED & 0x3F;
    uint32_t y_coord = fuse_chip->FUSE_OPT_Y_COORDINATE & 0x1FF;
    uint32_t x_coord = fuse_chip->FUSE_OPT_X_COORDINATE & 0x1FF;
    uint32_t wafer_id = fuse_chip->FUSE_OPT_WAFER_ID & 0x3F;
    uint32_t lot_code_0 = fuse_chip->FUSE_OPT_LOT_CODE_0;
    uint32_t lot_code_1 = fuse_chip->FUSE_OPT_LOT_CODE_1 & 0x0FFFFFFF;
    uint32_t fab_code = fuse_chip->FUSE_OPT_FAB_CODE & 0x3F;
    uint32_t vendor_code = fuse_chip->FUSE_OPT_VENDOR_CODE & 0xF;

    /* Hardware Info = OPS_RESERVED || Y_COORD || X_COORD || WAFER_ID || LOT_CODE || FAB_CODE || VENDOR_ID */
    hw_info[0] = (uint32_t)((lot_code_1 << 30) | (wafer_id << 24) | (x_coord << 15) | (y_coord << 6) | (ops_reserved));
    hw_info[1] = (uint32_t)((lot_code_0 << 26) | (lot_code_1 >> 2));
    hw_info[2] = (uint32_t)((fab_code << 26) | (lot_code_0 >> 6));
    hw_info[3] = (uint32_t)(vendor_code);

    memcpy(dst, hw_info, 0x10);
}

/* Check if have a new ODM fuse format. */
bool fuse_is_new_format(void) {
    return ((fuse_get_reserved_odm(4) & 0x800) && (fuse_get_reserved_odm(0) == 0x8E61ECAE) && (fuse_get_reserved_odm(1) == 0xF2BA3BB2));
}

/* Get the DeviceUniqueKeyGeneration. */
uint32_t fuse_get_device_unique_key_generation(void) {
    if (fuse_is_new_format()) {
        return (fuse_get_reserved_odm(2) & 0x1F);
    } else {
        return 0;
    }
}

/* Get the SocType from the HardwareType. */
uint32_t fuse_get_soc_type(void) {
    switch (fuse_get_hardware_type()) {
        case 0:
        case 1:
            return 0;           /* SocType_Erista */
        case 3:
        case 2:
        case 4:
        case 5:
            return 1;           /* SocType_Mariko */
        default:
            return 0xF;         /* SocType_Undefined */
    }
}

/* Get the Regulator type. */
uint32_t fuse_get_regulator(void) {
    if (fuse_get_soc_type() == 1) {
        return ((fuse_get_reserved_odm(28) & 1) + 1);   /* Regulator_Mariko_Max77812_A or Regulator_Mariko_Max77812_B */
    } else {
        return 0;               /* Regulator_Erista_Max77621 */
    }
}
