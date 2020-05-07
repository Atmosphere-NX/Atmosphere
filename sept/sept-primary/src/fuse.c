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

/* Prototypes for internal commands. */
void fuse_enable_power(void);
void fuse_disable_power(void);
void fuse_wait_idle(void);

/* Initialize the fuse driver */
void fuse_init(void) {
    /* Make all fuse registers visible. */
    clkrst_enable_fuse_regs(true);
}

/* Disable access to the private key and set the TZ sticky bit. */
void fuse_disable_private_key(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse->FUSE_PRIVATEKEYDISABLE = 0x10;
}

/* Disables all fuse programming. */
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
void fuse_wait_idle(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    uint32_t ctrl_val = 0;

    /* Wait for STATE_IDLE */
    while ((ctrl_val & (0xF0000)) != 0x40000)
        ctrl_val = fuse->FUSE_FUSECTRL;
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

/* Sense the fuse hardware array into the shadow cache. */
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

/* Read the SKU info register from the shadow cache. */
uint32_t fuse_get_sku_info(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    return fuse_chip->FUSE_SKU_INFO;
}

/* Read the bootrom patch version from a register in the shadow cache. */
uint32_t fuse_get_bootrom_patch_version(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    return fuse_chip->FUSE_SOC_SPEEDO_1_CALIB;
}

/* Read a spare bit register from the shadow cache */
uint32_t fuse_get_spare_bit(uint32_t idx) {
    if (idx < 32) {
        volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
        return fuse_chip->FUSE_SPARE_BIT[idx];
    } else {
        return 0;
    }
}

/* Read a reserved ODM register from the shadow cache. */
uint32_t fuse_get_reserved_odm(uint32_t idx) {
    if (idx < 8) {
        volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
        return fuse_chip->FUSE_RESERVED_ODM[idx];
    } else {
        return 0;
    }
}

/* Get the DRAM ID using values in the shadow cache. */
uint32_t fuse_get_dram_id(void) {
    return ((fuse_get_reserved_odm(4) >> 3) & 0x7);
}

/* Derive the Device ID using values in the shadow cache. */
uint64_t fuse_get_device_id(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();

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

/* Derive the Hardware Type using values in the shadow cache. */
uint32_t fuse_get_hardware_type(uint32_t target_firmware) {
    uint32_t fuse_reserved_odm4 = fuse_get_reserved_odm(4);
    uint32_t hardware_type = (((fuse_reserved_odm4 >> 7) & 2) | ((fuse_reserved_odm4 >> 2) & 1));

    /* Firmware from versions 1.0.0 to 3.0.2. */
    if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_4_0_0) {
        volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
        if (hardware_type >= 1) {
            return (hardware_type > 2) ? 3 : hardware_type - 1;
        } else if ((fuse_chip->FUSE_SPARE_BIT[9] & 1) == 0) {
            return 0;
        } else {
            return 3;
        }
    } else if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_7_0_0) {      /* Firmware versions from 4.0.0 to 6.2.0. */
        static const uint32_t types[] = {0,1,4,3};
        hardware_type |= ((fuse_reserved_odm4 >> 14) & 0x3C);
        hardware_type--;
        return (hardware_type > 3) ? 4 : types[hardware_type];
    } else {    /* Firmware versions from 7.0.0 onwards. */
        /* Always return 0 in retail. */
        return 0;
    }
}

/* Derive the Retail Type using values in the shadow cache. */
uint32_t fuse_get_retail_type(void) {
    /* Retail Type = IS_RETAIL | UNIT_TYPE. */
    uint32_t fuse_reserved_odm4 = fuse_get_reserved_odm(4);
    uint32_t retail_type = (((fuse_reserved_odm4 >> 7) & 4) | (fuse_reserved_odm4 & 3));
    if (retail_type == 4) { /* Standard retail unit, IS_RETAIL | 0. */
        return 1;
    } else if (retail_type == 3) { /* Standard dev unit, 0 | DEV_UNIT. */
        return 0;
    }
    return 2; /* IS_RETAIL | DEV_UNIT */
}

/* Derive the 16-byte Hardware Info using values in the shadow cache, and copy to output buffer. */
void fuse_get_hardware_info(void *dst) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
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
