/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_types.hpp"
#include "i2c_bus_accessor.hpp"

class I2cDriverSession {
    private:
        HosMutex bus_accessor_mutex;
        I2cBusAccessor *bus_accessor = nullptr;
        I2cBus bus = I2cBus_I2c1;
        u32 slave_address = 0;
        AddressingMode addressing_mode = AddressingMode_7Bit;
        u32 max_retries = 0;
        u64 retry_wait_time = 0;
        bool open = false;
    public:
        I2cDriverSession() {
            /* ... */
        }
    public:
        void Open(I2cBus bus, u32 slave_address, AddressingMode addr_mode, SpeedMode speed_mode, I2cBusAccessor *bus_accessor, u32 max_retries, u64 retry_wait_time);
        void Start();
        void Close();

        bool IsOpen() const;

        Result DoTransaction(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, DriverCommand command);
        Result DoTransactionWithRetry(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, DriverCommand command);
};
