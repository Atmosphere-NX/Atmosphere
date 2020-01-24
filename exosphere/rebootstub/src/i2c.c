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
 
#include "max77620.h"
#include "i2c.h"
#include "timer.h"

/* Prototypes for internal commands. */
void i2c_load_config(void);
int i2c_write(unsigned int device, uint32_t val, unsigned int num_bytes);
int i2c_read(unsigned int device, void *dst, unsigned num_bytes);
int i2c_query(uint8_t device, uint8_t r, void *dst, size_t num_bytes); 
int i2c_send_byte_command(unsigned int device, unsigned char reg, unsigned char b);

/* Load hardware config for I2C4. */
void i2c_load_config(void) {
    /* Set MSTR_CONFIG_LOAD, TIMEOUT_CONFIG_LOAD, undocumented bit. */
    I2C_I2C_CONFIG_LOAD_0 = 0x25;

    /* Wait a bit for master config to be loaded. */
    for (unsigned int i = 0; i < 20; i++) {
        timer_wait(1);
        if (!(I2C_I2C_CONFIG_LOAD_0 & 1)) {
            break;
        }
    }
}

/* Initialize I2C4. */
void i2c_init(void) {
    /* Setup divisor, and clear the bus. */
    I2C_I2C_CLK_DIVISOR_REGISTER_0 = 0x50001;
    I2C_I2C_BUS_CLEAR_CONFIG_0 = 0x90003;

    /* Load hardware configuration. */
    i2c_load_config();

    /* Wait a while until BUS_CLEAR_DONE is set. */
    for (unsigned int i = 0; i < 10; i++) {
        timer_wait(20000);
        if (I2C_INTERRUPT_STATUS_REGISTER_0 & 0x800) {
            break;
        }
    }

    /* Read the BUS_CLEAR_STATUS. Result doesn't matter. */
    I2C_I2C_BUS_CLEAR_STATUS_0;

    /* Read and set the Interrupt Status. */
    uint32_t int_status = I2C_INTERRUPT_STATUS_REGISTER_0;
    I2C_INTERRUPT_STATUS_REGISTER_0 = int_status;
}

/* Writes a value to an i2c device. */
int i2c_write(unsigned int device, uint32_t val, unsigned int num_bytes) {
    if (num_bytes > 4) {
        return 0;
    }

    /* Set device for 7-bit mode. */
    I2C_I2C_CMD_ADDR0_0 = device << 1;

    /* Load in data to write. */
    I2C_I2C_CMD_DATA1_0 = val;

    /* Set config with LENGTH = num_bytes, NEW_MASTER_FSM, DEBOUNCE_CNT = 4T. */
    I2C_I2C_CNFG_0 = ((num_bytes << 1) - 2) | 0x2800;

    i2c_load_config();

    /* Config |= SEND; */
    I2C_I2C_CNFG_0 |= 0x200;


    while (I2C_I2C_STATUS_0 & 0x100) {
        /* Wait until not busy. */
    }

    /* Return CMD1_STAT == SL1_XFER_SUCCESSFUL. */
    return (I2C_I2C_STATUS_0 & 0xF) == 0;
}

/* Reads a value from an i2c device. */
int i2c_read(unsigned device, void *dst, unsigned num_bytes) {
    if (num_bytes > 4) {
        return 0;
    }

    /* Set device for 7-bit read mode. */
    I2C_I2C_CMD_ADDR0_0 = (device << 1) | 1;

    /* Set config with LENGTH = num_bytes, NEW_MASTER_FSM, DEBOUNCE_CNT = 4T. */
    I2C_I2C_CNFG_0 = ((num_bytes << 1) - 2) | 0x2840;

    i2c_load_config();

    /* Config |= SEND; */
    I2C_I2C_CNFG_0 |= 0x200;


    while (I2C_I2C_STATUS_0 & 0x100) {
        /* Wait until not busy. */
    }
    
    /* Ensure success. */
    if ((I2C_I2C_STATUS_0 & 0xF) != 0) {
        return 0;
    }
    
    uint32_t val = I2C_I2C_CMD_DATA1_0;
    for (size_t i = 0; i < num_bytes; i++) {
        ((uint8_t *)dst)[i] = ((uint8_t *)&val)[i];
    }
    return 1;
}

/* Queries the value of a register. */
int i2c_query(uint8_t device, uint8_t r, void *dst, size_t num_bytes) {
    /* Limit output size to 32-bits. */
    if (num_bytes > 4) {
        return 0;
    }
    
    /* Write single byte register ID to device. */
    if (!i2c_write(device, r, 1)) {
        return 0;
    }
    
    return i2c_read(device, dst, num_bytes);

}

/* Writes a byte val to reg for given device. */
int i2c_send_byte_command(unsigned int device, unsigned char reg, unsigned char b) {
    uint32_t val = (reg) | (b << 8);
    /* Write 1 byte (reg) + 1 byte (value) */
    return i2c_write(device, val, 2);
}

void i2c_stop_rtc_alarm(void) {
    i2c_send_byte_command(MAX77620_RTC_I2C_ADDR, MAX77620_REG_RTCUPDATE0, 0x10);
    
    uint8_t val = 0;
    for (int i = 0; i < 14; i++) {
        if (i2c_query(MAX77620_RTC_I2C_ADDR, 0x0E + i, &val, 1)) {
            val &= 0x7F;
            i2c_send_byte_command(MAX77620_RTC_I2C_ADDR, 0x0E + i, val);
        }
    }
        
    i2c_send_byte_command(MAX77620_RTC_I2C_ADDR, MAX77620_REG_RTCUPDATE0, 0x01);
}

void i2c_send_shutdown_cmd(void) {
    i2c_send_byte_command(MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_PWR_OFF);
}
