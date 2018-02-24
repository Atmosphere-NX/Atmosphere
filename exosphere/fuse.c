#include <stbool.h>
#include <stdint.h>
#include <string.h>

#include "fuse.h"
#include "utils.h"
#include "timers.h"

/* Prototypes for internal commands. */
void fuse_make_regs_visible(void);

void fuse_enable_power(void);
void fuse_disable_power(void);
void fuse_wait_idle(void);

/* Initialize the FUSE driver */
void fuse_init(void)
{
    fuse_make_regs_visible();
    
    /* TODO: Overrides (iROM patches) and various reads happen here */
}

/* Make all fuse registers visible */
void fuse_make_regs_visible(void)
{
    /* TODO: Replace this with a proper CLKRST driver */
    uint32_t* misc_clk_reg = (volatile uint32_t *)mmio_get_device_address(MMIO_DEVID_CLKRST) + 0x48;
    uint32_t misc_clk_val = *misc_clk_reg;
    *misc_clk_reg = (misc_clk_val | (1 << 28));
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
void fuse_hw_write(uint32_t, value, uint32_t addr)
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
    uint32_t spare_bit_val = 0;
    
    if ((idx >= 0) && (idx < 32))
        spare_bit_val = FUSE_CHIP_REGS->FUSE_SPARE_BIT[idx];
    
    return spare_bit_val;
}

/* Read a reserved ODM register from the shadow cache */
uint32_t fuse_get_reserved_odm(uint32_t idx)
{
    uint32_t reserved_odm_val = 0;
    
    if ((idx >= 0) && (idx < 8))
        reserved_odm_val = FUSE_CHIP_REGS->FUSE_RESERVED_ODM[idx];
    
    return reserved_odm_val;
}