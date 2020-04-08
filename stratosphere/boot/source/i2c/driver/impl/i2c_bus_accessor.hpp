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
#include "i2c_registers.hpp"

namespace ams::i2c::driver::impl {

    class BusAccessor {
        private:
            enum class TransferMode {
                Send    = 0,
                Receive = 1,
            };
            static constexpr TimeSpan InterruptTimeout = TimeSpan::FromMilliSeconds(100);
        private:
            os::InterruptEventType interrupt_event;
            os::Mutex open_mutex;
            os::Mutex register_mutex;
            Registers *i2c_registers = nullptr;
            ClkRstRegisters clkrst_registers;
            SpeedMode speed_mode = SpeedMode::Fast;
            size_t open_sessions = 0;
            Bus bus = Bus::I2C1;
            PcvModule pcv_module = PcvModule_I2C1;
            bool suspended = false;
        public:
            BusAccessor() : open_mutex(false), register_mutex(false) { /* ... */ }
        private:
            inline void ClearInterruptMask() const {
                reg::Write(&i2c_registers->I2C_INTERRUPT_MASK_REGISTER_0, 0);
                reg::Read(&i2c_registers->I2C_INTERRUPT_MASK_REGISTER_0);
            }

            void SetBus(Bus bus);
            void CreateInterruptEvent(Bus bus);
            void SetClock(SpeedMode speed_mode);

            void ResetController() const;
            void ClearBus() const;
            void DisableClock();
            void SetPacketMode();
            Result FlushFifos();

            Result GetTransactionResult() const;
            void HandleTransactionResult(Result result);
            Result GetAndHandleTransactionResult();

            void WriteTransferHeader(TransferMode transfer_mode, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address, size_t num_bytes);
        public:
            void Open(Bus bus, SpeedMode speed_mode);
            void Close();
            void Suspend();
            void Resume();
            void DoInitialConfig();

            size_t GetOpenSessions() const;
            bool GetBusy() const;

            void OnStartTransaction() const;
            Result StartTransaction(Command command, AddressingMode addressing_mode, u32 slave_address);
            Result Send(const u8 *data, size_t num_bytes, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address);
            Result Receive(u8 *out_data, size_t num_bytes, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address);
            void OnStopTransaction() const;
    };


}