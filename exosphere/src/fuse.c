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
 
#include <string.h>

#include "car.h"
#include "fuse.h"
#include "utils.h"
#include "timers.h"
#include "exocfg.h"

#include "masterkey.h"
/* Prototypes for internal commands. */
void fuse_make_regs_visible(void);

void fuse_enable_power(void);
void fuse_disable_power(void);
void fuse_wait_idle(void);

/* Initialize the FUSE driver */
void fuse_init(void)
{
    fuse_make_regs_visible();
    fuse_secondary_private_key_disable();
    fuse_disable_programming();

    /* TODO: Overrides (iROM patches) and various reads happen here */
}

/* Make all fuse registers visible */
void fuse_make_regs_visible(void)
{
    CLK_RST_CONTROLLER_MISC_CLK_ENB_0 |= BIT(28);
}

/* Enable power to the fuse hardware array */
void fuse_enable_power(void)
{
    FUSE_REGS->FUSE_PWR_GOOD_SW = 1;
    wait(1);
}

/* Disable power to the fuse hardware array */
void fuse_disable_power(void)
{
    FUSE_REGS->FUSE_PWR_GOOD_SW = 0;
    wait(1);
}

/* Wait for the fuse driver to go idle */
void fuse_wait_idle(void)
{
    uint32_t ctrl_val = 0;

    /* Wait for STATE_IDLE */
    while ((ctrl_val & (0xF0000)) != 0x40000)
    {
        wait(1);
        ctrl_val = FUSE_REGS->FUSE_CTRL;
    }
}

/* Read a fuse from the hardware array */
uint32_t fuse_hw_read(uint32_t addr)
{
    fuse_wait_idle();

    /* Program the target address */
    FUSE_REGS->FUSE_REG_ADDR = addr;

    /* Enable read operation in control register */
    uint32_t ctrl_val = FUSE_REGS->FUSE_CTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x1;    /* Set FUSE_READ command */
    FUSE_REGS->FUSE_CTRL = ctrl_val;

    fuse_wait_idle();

    return FUSE_REGS->FUSE_REG_READ;
}

/* Write a fuse in the hardware array */
void fuse_hw_write(uint32_t value, uint32_t addr)
{
    fuse_wait_idle();

    /* Program the target address and value */
    FUSE_REGS->FUSE_REG_ADDR = addr;
    FUSE_REGS->FUSE_REG_WRITE = value;

    /* Enable write operation in control register */
    uint32_t ctrl_val = FUSE_REGS->FUSE_CTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x2;    /* Set FUSE_WRITE command */
    FUSE_REGS->FUSE_CTRL = ctrl_val;

    fuse_wait_idle();
}

/* Sense the fuse hardware array into the shadow cache */
void fuse_hw_sense(void)
{
    fuse_wait_idle();

    /* Enable sense operation in control register */
    uint32_t ctrl_val = FUSE_REGS->FUSE_CTRL;
    ctrl_val &= ~0x3;
    ctrl_val |= 0x3;    /* Set FUSE_SENSE command */
    FUSE_REGS->FUSE_CTRL = ctrl_val;

    fuse_wait_idle();
}

/* Disables all fuse programming. */
void fuse_disable_programming(void) {
    FUSE_REGS->FUSE_DIS_PGM = 1;
}

/* Unknown exactly what this does, but it alters the contents read from the fuse cache. */
void fuse_secondary_private_key_disable(void) {
    FUSE_REGS->FUSE_PRIVATEKEYDISABLE = 0x10;
}


/* Read the SKU info register from the shadow cache */
uint32_t fuse_get_sku_info(void)
{
    return FUSE_CHIP_REGS->FUSE_SKU_INFO;
}

/* Read the bootrom patch version from a register in the shadow cache */
uint32_t fuse_get_bootrom_patch_version(void)
{
    return FUSE_CHIP_REGS->FUSE_SOC_SPEEDO_1;
}

/* Read a spare bit register from the shadow cache */
uint32_t fuse_get_spare_bit(uint32_t idx)
{
    if (idx >= 32) {
        return 0;
    }

    return FUSE_CHIP_REGS->FUSE_SPARE_BIT[idx];
}

/* Read a reserved ODM register from the shadow cache */
uint32_t fuse_get_reserved_odm(uint32_t idx)
{
    if (idx >= 8) {
        return 0;
    }

    return FUSE_CHIP_REGS->FUSE_RESERVED_ODM[idx];
}

uint32_t fuse_get_5x_key_generation(void) {
    if ((fuse_get_reserved_odm(4) & 0x800) && fuse_get_reserved_odm(0) == 0x8E61ECAE && fuse_get_reserved_odm(1) == 0xF2BA3BB2) {
        return fuse_get_reserved_odm(2) & 0x1F;
    } else {
        return 0;
    }
}

/* Derive the Device ID using values in the shadow cache */
uint64_t fuse_get_device_id(void) {
    uint64_t device_id = 0;
    uint64_t y_coord = FUSE_CHIP_REGS->FUSE_Y_COORDINATE & 0x1FF;
    uint64_t x_coord = FUSE_CHIP_REGS->FUSE_X_COORDINATE & 0x1FF;
    uint64_t wafer_id = FUSE_CHIP_REGS->FUSE_WAFER_ID & 0x3F;
    uint32_t lot_code = FUSE_CHIP_REGS->FUSE_LOT_CODE_0;
    uint64_t fab_code = FUSE_CHIP_REGS->FUSE_FAB_CODE & 0x3F;
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
    return (FUSE_CHIP_REGS->FUSE_RESERVED_ODM[4] >> 3) & 0x7;
}

/* Derive the Hardware Type using values in the shadow cache */
uint32_t fuse_get_hardware_type(void) {
    /* This function is very different between 4.x and < 4.x */
    uint32_t hardware_type = ((FUSE_CHIP_REGS->FUSE_RESERVED_ODM[4] >> 7) & 2) | ((FUSE_CHIP_REGS->FUSE_RESERVED_ODM[4] >> 2) & 1);
    
    if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
        static const uint32_t types[] = {0,1,4,3};

        hardware_type |= (FUSE_CHIP_REGS->FUSE_RESERVED_ODM[4] >> 14) & 0x3C;
        hardware_type--;
        return hardware_type > 3 ? 4 : types[hardware_type];
    } else {
        if (hardware_type >= 1) {
            return hardware_type > 2 ? 3 : hardware_type - 1; 
        } else if ((FUSE_CHIP_REGS->FUSE_SPARE_BIT[9] & 1) == 0) {
            return 0;
        } else {
            return 3;
        }
    }
}

/* Derive the Retail Type using values in the shadow cache */
uint32_t fuse_get_retail_type(void) {
    /* Retail type = IS_RETAIL | UNIT_TYPE */
    uint32_t retail_type = ((FUSE_CHIP_REGS->FUSE_RESERVED_ODM[4] >> 7) & 4) | (FUSE_CHIP_REGS->FUSE_RESERVED_ODM[4] & 3);
    if (retail_type == 4) { /* Standard retail unit, IS_RETAIL | 0. */
        return 1;
    } else if (retail_type == 3) { /* Standard dev unit, 0 | DEV_UNIT. */
        return 0;
    }
    return 2; /* IS_RETAIL | DEV_UNIT */
}

/* Derive the 16-byte Hardware Info using values in the shadow cache, and copy to output buffer. */
void fuse_get_hardware_info(void *dst) {
    uint32_t hw_info[0x4];

    uint32_t unk_hw_fuse = FUSE_CHIP_REGS->_0x120 & 0x3F;
    uint32_t y_coord = FUSE_CHIP_REGS->FUSE_Y_COORDINATE & 0x1FF;
    uint32_t x_coord = FUSE_CHIP_REGS->FUSE_X_COORDINATE & 0x1FF;
    uint32_t wafer_id = FUSE_CHIP_REGS->FUSE_WAFER_ID & 0x3F;
    uint32_t lot_code_0 = FUSE_CHIP_REGS->FUSE_LOT_CODE_0;
    uint32_t lot_code_1 = FUSE_CHIP_REGS->FUSE_LOT_CODE_1 & 0x0FFFFFFF;
    uint32_t fab_code = FUSE_CHIP_REGS->FUSE_FAB_CODE & 0x3F;
    uint32_t vendor_code = FUSE_CHIP_REGS->FUSE_VENDOR_CODE & 0xF;

    /* Hardware Info = unk_hw_fuse || Y_COORD || X_COORD || WAFER_ID || LOT_CODE || FAB_CODE || VENDOR_ID */
    hw_info[0] = (uint32_t)((lot_code_1 << 30) | (wafer_id << 24) | (x_coord << 15) | (y_coord << 6) | (unk_hw_fuse));
    hw_info[1] = (uint32_t)((lot_code_0 << 26) | (lot_code_1 >> 2));
    hw_info[2] = (uint32_t)((fab_code << 26) | (lot_code_0 >> 6));
    hw_info[3] = (uint32_t)(vendor_code);

    memcpy(dst, hw_info, 0x10);
}
