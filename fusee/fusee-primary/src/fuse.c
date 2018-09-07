/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "car.h"
#include "fuse.h"
#include "timers.h"

/* Prototypes for internal commands. */
void fuse_make_regs_visible(void);

void fuse_enable_power(void);
void fuse_disable_power(void);
void fuse_wait_idle(void);

/* Initialize the fuse driver */
void fuse_init(void) {
    fuse_make_regs_visible();
    fuse_secondary_private_key_disable();
    fuse_disable_programming();

    /* TODO: Overrides (iROM patches) and various reads happen here */
}

/* Make all fuse registers visible */
void fuse_make_regs_visible(void) {
    clkrst_enable_fuse_regs(true);
}

/* Enable power to the fuse hardware array */
void fuse_enable_power(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse->FUSE_PWR_GOOD_SW = 1;
    udelay(1);
}

/* Disable power to the fuse hardware array */
void fuse_disable_power(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse->FUSE_PWR_GOOD_SW = 0;
    udelay(1);
}

/* Wait for the fuse driver to go idle */
void fuse_wait_idle(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    uint32_t ctrl_val = 0;

    /* Wait for STATE_IDLE */
    while ((ctrl_val & (0xF0000)) != 0x40000)
    {
        udelay(1);
        ctrl_val = fuse->FUSE_CTRL;
    }
}

/* Read a fuse from the hardware array */
uint32_t fuse_hw_read(uint32_t addr) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse_wait_idle();

    /* Program the target address */
    fuse->FUSE_REG_ADDR = addr;

    /* Enable read operation in control register */
    uint32_t ctrl_val = fuse->FUSE_CTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x1;    /* Set FUSE_READ command */
    fuse->FUSE_CTRL = ctrl_val;

    fuse_wait_idle();

    return fuse->FUSE_REG_READ;
}

/* Write a fuse in the hardware array */
void fuse_hw_write(uint32_t value, uint32_t addr) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse_wait_idle();

    /* Program the target address and value */
    fuse->FUSE_REG_ADDR = addr;
    fuse->FUSE_REG_WRITE = value;

    /* Enable write operation in control register */
    uint32_t ctrl_val = fuse->FUSE_CTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x2;    /* Set FUSE_WRITE command */
    fuse->FUSE_CTRL = ctrl_val;

    fuse_wait_idle();
}

/* Sense the fuse hardware array into the shadow cache */
void fuse_hw_sense(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse_wait_idle();

    /* Enable sense operation in control register */
    uint32_t ctrl_val = fuse->FUSE_CTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x3;    /* Set FUSE_SENSE command */
    fuse->FUSE_CTRL = ctrl_val;

    fuse_wait_idle();
}

/* Disables all fuse programming. */
void fuse_disable_programming(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse->FUSE_DIS_PGM = 1;
}

/* Unknown exactly what this does, but it alters the contents read from the fuse cache. */
void fuse_secondary_private_key_disable(void) {
    volatile tegra_fuse_t *fuse = fuse_get_regs();
    fuse->FUSE_PRIVATEKEYDISABLE = 0x10;
}


/* Read the SKU info register from the shadow cache */
uint32_t fuse_get_sku_info(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    return fuse_chip->FUSE_SKU_INFO;
}

/* Read the bootrom patch version from a register in the shadow cache */
uint32_t fuse_get_bootrom_patch_version(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    return fuse_chip->FUSE_SOC_SPEEDO_1;
}

/* Read a spare bit register from the shadow cache */
uint32_t fuse_get_spare_bit(uint32_t idx) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    
    if (idx >= 32) {
        return 0;
    }

    return fuse_chip->FUSE_SPARE_BIT[idx];
}

/* Read a reserved ODM register from the shadow cache */
uint32_t fuse_get_reserved_odm(uint32_t idx) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    
    if (idx >= 8) {
        return 0;
    }

    return fuse_chip->FUSE_RESERVED_ODM[idx];
}

/* Derive the Device ID using values in the shadow cache */
uint64_t fuse_get_device_id(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    
    uint64_t device_id = 0;
    uint64_t y_coord = fuse_chip->FUSE_Y_COORDINATE & 0x1FF;
    uint64_t x_coord = fuse_chip->FUSE_X_COORDINATE & 0x1FF;
    uint64_t wafer_id = fuse_chip->FUSE_WAFER_ID & 0x3F;
    uint32_t lot_code = fuse_chip->FUSE_LOT_CODE_0;
    uint64_t fab_code = fuse_chip->FUSE_FAB_CODE & 0x3F;
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

/* Get the DRAM ID using values in the shadow cache */
uint32_t fuse_get_dram_id(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    return (fuse_chip->FUSE_RESERVED_ODM[4] >> 3) & 0x7;
}

/* Derive the Hardware Type using values in the shadow cache */
uint32_t fuse_get_hardware_type(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    
    /* This function is very different between 4.x and < 4.x */
    uint32_t hardware_type = ((fuse_chip->FUSE_RESERVED_ODM[4] >> 7) & 2) | ((fuse_chip->FUSE_RESERVED_ODM[4] >> 2) & 1);

   /* TODO: choose; if (mkey_get_revision() >= MASTERKEY_REVISION_400_CURRENT) {
        static const uint32_t types[] = {0,1,4,3};

        hardware_type |= (fuse_chip->FUSE_RESERVED_ODM[4] >> 14) & 0x3C;
        hardware_type--;
        return hardware_type > 3 ? 4 : types[hardware_type];
    } else {*/
        if (hardware_type >= 1) {
            return hardware_type > 2 ? 3 : hardware_type - 1;
        } else if ((fuse_chip->FUSE_SPARE_BIT[9] & 1) == 0) {
            return 0;
        } else {
            return 3;
        }
//    }
}

/* Derive the Retail Type using values in the shadow cache */
uint32_t fuse_get_retail_type(void) {
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    
    /* Retail type = IS_RETAIL | UNIT_TYPE */
    uint32_t retail_type = ((fuse_chip->FUSE_RESERVED_ODM[4] >> 7) & 4) | (fuse_chip->FUSE_RESERVED_ODM[4] & 3);
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

    uint32_t unk_hw_fuse = fuse_chip->_0x120 & 0x3F;
    uint32_t y_coord = fuse_chip->FUSE_Y_COORDINATE & 0x1FF;
    uint32_t x_coord = fuse_chip->FUSE_X_COORDINATE & 0x1FF;
    uint32_t wafer_id = fuse_chip->FUSE_WAFER_ID & 0x3F;
    uint32_t lot_code_0 = fuse_chip->FUSE_LOT_CODE_0;
    uint32_t lot_code_1 = fuse_chip->FUSE_LOT_CODE_1 & 0x0FFFFFFF;
    uint32_t fab_code = fuse_chip->FUSE_FAB_CODE & 0x3F;
    uint32_t vendor_code = fuse_chip->FUSE_VENDOR_CODE & 0xF;

    /* Hardware Info = unk_hw_fuse || Y_COORD || X_COORD || WAFER_ID || LOT_CODE || FAB_CODE || VENDOR_ID */
    hw_info[0] = (uint32_t)((lot_code_1 << 30) | (wafer_id << 24) | (x_coord << 15) | (y_coord << 6) | (unk_hw_fuse));
    hw_info[1] = (uint32_t)((lot_code_0 << 26) | (lot_code_1 >> 2));
    hw_info[2] = (uint32_t)((fab_code << 26) | (lot_code_0 >> 6));
    hw_info[3] = (uint32_t)(vendor_code);

    memcpy(dst, hw_info, 0x10);
}
