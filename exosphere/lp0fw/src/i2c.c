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
#include "timer.h"

/* Prototypes for internal commands. */
void i2c_set_test_master_config_load(void);
void i2c_write(unsigned int device, uint32_t val, unsigned int num_bytes);
void i2c_send_byte_command(unsigned int device, unsigned char reg, unsigned char b);

/* Load hardware config for I2C5. */
void i2c_set_test_master_config_load(void) {
    /* Set MSTR_CONFIG_LOAD. */
    I2C_I2C_CONFIG_LOAD_0 = 0x1;
    
    while (I2C_I2C_CONFIG_LOAD_0 & 1) {
        /* Wait forever until it's unset. */
    }
}

/* Writes a value to an i2c device. */
void i2c_write(unsigned int device, uint32_t val, unsigned int num_bytes) {
    if (num_bytes > 4) {
        return;
    }

    /* Set device for 7-bit mode. */
    I2C_I2C_CMD_ADDR0_0 = device << 1;

    /* Load in data to write. */
    I2C_I2C_CMD_DATA1_0 = val;

    /* Set config with LENGTH = num_bytes, NEW_MASTER_FSM, DEBOUNCE_CNT = 4T. */
    I2C_I2C_CNFG_0 = ((num_bytes << 1) - 2) | 0x2800;

    i2c_set_test_master_config_load();

    /* Config |= SEND; */
    I2C_I2C_CNFG_0 = ((num_bytes << 1) - 2) | 0x2800 | 0x200;


    while (I2C_I2C_STATUS_0 & 0x100) {
        /* Wait until not busy. */
    }
    
    while (I2C_I2C_STATUS_0 & 0xF) {
        /* Wait until write successful. */
    }
}

/* Writes a byte val to reg for given device. */
void i2c_send_byte_command(unsigned int device, unsigned char reg, unsigned char b) {
    uint32_t val = (reg) | (b << 8);
    /* Write 1 byte (reg) + 1 byte (value) */
    i2c_write(device, val, 2);
}

/* Enable the PMIC. */
void i2c_enable_pmic(void) {
    /* Write 00 to Device 27 Reg 00. */
    i2c_send_byte_command(27, 0, 0x80);
}
