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
#pragma once
#include "i2c_driver_types.hpp"
#include "i2c_bus_accessor.hpp"

namespace ams::i2c::driver::impl {

    class Session {
        private:
            os::Mutex bus_accessor_mutex;
            BusAccessor *bus_accessor = nullptr;
            Bus bus = Bus::I2C1;
            u32 slave_address = 0;
            AddressingMode addressing_mode = AddressingMode::SevenBit;
            u32 max_retries = 0;
            u64 retry_wait_time = 0;
            bool open = false;
        public:
            Session() : bus_accessor_mutex(false) { /* ... */ }
        public:
            void Open(Bus bus, u32 slave_address, AddressingMode addr_mode, SpeedMode speed_mode, BusAccessor *bus_accessor, u32 max_retries, u64 retry_wait_time);
            void Start();
            void Close();

            bool IsOpen() const;

            Result DoTransaction(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, Command command);
            Result DoTransactionWithRetry(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, Command command);
    };

}


