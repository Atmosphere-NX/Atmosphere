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
 
#include "i2c.h"
#include "utils.h"
#include "timers.h"
#include "pinmux.h"

/* Prototypes for internal commands. */
volatile tegra_i2c_t *i2c_get_registers_from_id(I2CDevice id);
void i2c_load_config(volatile tegra_i2c_t *regs);

bool i2c_query(I2CDevice id, uint8_t device, uint8_t r, void *dst, size_t dst_size);
bool i2c_send(I2CDevice id, uint8_t device, uint8_t r, void *src, size_t src_size);

bool i2c_write(volatile tegra_i2c_t *regs, uint8_t device, void *src, size_t src_size);
bool i2c_read(volatile tegra_i2c_t *regs, uint8_t device, void *dst, size_t dst_size);

/* Configure I2C pinmux. */
void i2c_config(I2CDevice id) {
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    
    switch (id) {
        case I2C_1:
            pinmux->gen1_i2c_scl = PINMUX_INPUT;
            pinmux->gen1_i2c_sda = PINMUX_INPUT;
            break;
        case I2C_2:
            pinmux->gen2_i2c_scl = PINMUX_INPUT;
            pinmux->gen2_i2c_sda = PINMUX_INPUT;
            break;
        case I2C_3:
            pinmux->gen3_i2c_scl = PINMUX_INPUT;
            pinmux->gen3_i2c_sda = PINMUX_INPUT;
            break;
        case I2C_4:
            pinmux->cam_i2c_scl = PINMUX_INPUT;
            pinmux->cam_i2c_sda = PINMUX_INPUT;
            break;
        case I2C_5:
            pinmux->pwr_i2c_scl = PINMUX_INPUT;
            pinmux->pwr_i2c_sda = PINMUX_INPUT;
            break;
        case I2C_6:
            /* Unused. */
            break;
        default: break;
    }
}

/* Initialize I2C based on registers. */
void i2c_init(I2CDevice id) {
    volatile tegra_i2c_t *regs = i2c_get_registers_from_id(id);

    /* Setup divisor, and clear the bus. */
    regs->I2C_I2C_CLK_DIVISOR_REGISTER_0 = 0x50001;
    regs->I2C_I2C_BUS_CLEAR_CONFIG_0 = 0x90003;

    /* Load hardware configuration. */
    i2c_load_config(regs);

    /* Wait a while until BUS_CLEAR_DONE is set. */
    for (unsigned int i = 0; i < 10; i++) {
        udelay(20000);
        if (regs->I2C_INTERRUPT_STATUS_REGISTER_0 & 0x800) {
            break;
        }
    }

    /* Read the BUS_CLEAR_STATUS. Result doesn't matter. */
    regs->I2C_I2C_BUS_CLEAR_STATUS_0;

    /* Read and set the Interrupt Status. */
    uint32_t int_status = regs->I2C_INTERRUPT_STATUS_REGISTER_0;
    regs->I2C_INTERRUPT_STATUS_REGISTER_0 = int_status;
}

/* Sets a bit in a PMIC register over I2C during CPU shutdown. */ 
void i2c_send_pmic_cpu_shutdown_cmd(void) {
    uint32_t val = 0;
    /* PMIC == Device 4:3C. */
    i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, 0x41, &val, 1);
    val |= 4;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, 0x41, &val, 1);
}

/* Queries the value of TI charger bit over I2C. */
bool i2c_query_ti_charger_bit_7(void) {
    uint32_t val = 0;
    /* TI Charger = Device 0:6B. */
    i2c_query(I2C_1, BQ24193_I2C_ADDR, 0, &val, 1);
    return (val & 0x80) != 0;
}

/* Clears TI charger bit over I2C. */
void i2c_clear_ti_charger_bit_7(void) {
    uint32_t val = 0;
    /* TI Charger = Device 0:6B. */
    i2c_query(I2C_1, BQ24193_I2C_ADDR, 0, &val, 1);
    val &= 0x7F;
    i2c_send(I2C_1, BQ24193_I2C_ADDR, 0, &val, 1);
}

/* Sets TI charger bit over I2C. */
void i2c_set_ti_charger_bit_7(void) {
    uint32_t val = 0;
    /* TI Charger = Device 0:6B. */
    i2c_query(I2C_1, BQ24193_I2C_ADDR, 0, &val, 1);
    val |= 0x80;
    i2c_send(I2C_1, BQ24193_I2C_ADDR, 0, &val, 1);
}

/* Get registers pointer based on I2C ID. */
volatile tegra_i2c_t *i2c_get_registers_from_id(I2CDevice id) {
    switch (id) {
        case I2C_1:
            return I2C1_REGS;
        case I2C_2:
            return I2C2_REGS;
        case I2C_3:
            return I2C3_REGS;
        case I2C_4:
            return I2C4_REGS;
        case I2C_5:
            return I2C5_REGS;
        case I2C_6:
            return I2C6_REGS;
        default:
            generic_panic();
    }
    return NULL;
}

/* Load hardware config for I2C4. */
void i2c_load_config(volatile tegra_i2c_t *regs) {
    /* Set MSTR_CONFIG_LOAD, TIMEOUT_CONFIG_LOAD, undocumented bit. */
    regs->I2C_I2C_CONFIG_LOAD_0 = 0x25;

    /* Wait a bit for master config to be loaded. */
    for (unsigned int i = 0; i < 20; i++) {
        udelay(1);
        if (!(regs->I2C_I2C_CONFIG_LOAD_0 & 1)) {
            break;
        }
    }
}

/* Reads a register from a device over I2C, writes result to output. */
bool i2c_query(I2CDevice id, uint8_t device, uint8_t r, void *dst, size_t dst_size) {
    volatile tegra_i2c_t *regs = i2c_get_registers_from_id(id);
    uint32_t val = r;
    
    /* Write single byte register ID to device. */
    if (!i2c_write(regs, device, &val, 1)) {
        return false;
    }
    /* Limit output size to 32-bits. */
    if (dst_size > 4) {
        return false;
    }
    
    return i2c_read(regs, device, dst, dst_size);
}

/* Writes a value to a register over I2C. */
bool i2c_send(I2CDevice id, uint8_t device, uint8_t r, void *src, size_t src_size) {    
    uint32_t val = r;
    if (src_size == 0) {
        return true;
    } else if (src_size <= 3) {
        memcpy(((uint8_t *)&val) + 1, src, src_size);
        return i2c_write(i2c_get_registers_from_id(id), device, &val, src_size + 1);
    } else {
        return false;
    }
}

/* Writes bytes to device over I2C. */
bool i2c_write(volatile tegra_i2c_t *regs, uint8_t device, void *src, size_t src_size) {
    if (src_size > 4) {
        return false;
    } else if (src_size == 0) {
        return true;
    }

    /* Set device for 7-bit write mode. */
    regs->I2C_I2C_CMD_ADDR0_0 = device << 1;

    /* Load in data to write. */
    regs->I2C_I2C_CMD_DATA1_0 = read32le(src, 0);

    /* Set config with LENGTH = src_size, NEW_MASTER_FSM, DEBOUNCE_CNT = 4T. */
    regs->I2C_I2C_CNFG_0 = ((src_size << 1) - 2) | 0x2800;

    i2c_load_config(regs);

    /* Config |= SEND; */
    regs->I2C_I2C_CNFG_0 = ((regs->I2C_I2C_CNFG_0 & 0xFFFFFDFF) | 0x200);

    while (regs->I2C_I2C_STATUS_0 & 0x100) {
        /* Wait until not busy. */
    }

    /* Return CMD1_STAT == SL1_XFER_SUCCESSFUL. */
    return (regs->I2C_I2C_STATUS_0 & 0xF) == 0;
}

/* Reads bytes from device over I2C. */
bool i2c_read(volatile tegra_i2c_t *regs, uint8_t device, void *dst, size_t dst_size) {
    if (dst_size > 4) {
        return false;
    } else if (dst_size == 0) {
        return true;
    }

    /* Set device for 7-bit read mode. */
    regs->I2C_I2C_CMD_ADDR0_0 = (device << 1) | 1;

    /* Set config with LENGTH = dst_size, NEW_MASTER_FSM, DEBOUNCE_CNT = 4T. */
    regs->I2C_I2C_CNFG_0 = ((dst_size << 1) - 2) | 0x2840;

    i2c_load_config(regs);

    /* Config |= SEND; */
    regs->I2C_I2C_CNFG_0 = ((regs->I2C_I2C_CNFG_0 & 0xFFFFFDFF) | 0x200);

    while (regs->I2C_I2C_STATUS_0 & 0x100) {
        /* Wait until not busy. */
    }
    
    /* Ensure success. */
    if ((regs->I2C_I2C_STATUS_0 & 0xF) != 0) {
        return false;
    }

    uint32_t val = regs->I2C_I2C_CMD_DATA1_0;
    memcpy(dst, &val, dst_size);
    return true;
}
