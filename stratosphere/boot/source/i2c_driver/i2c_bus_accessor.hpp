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
#include "i2c_registers.hpp"

class I2cBusAccessor {
    private:
        enum TransferMode {
            TransferMode_Send = 0,
            TransferMode_Receive = 1,
        };
        static constexpr u64 InterruptTimeout = 100'000'000ul;
    private:
        Event interrupt_event;
        HosMutex open_mutex;
        HosMutex register_mutex;
        I2cRegisters *i2c_registers = nullptr;
        ClkRstRegisters clkrst_registers;
        SpeedMode speed_mode = SpeedMode_Fast;
        size_t open_sessions = 0;
        I2cBus bus = I2cBus_I2c1;
        PcvModule pcv_module = PcvModule_I2C1;
        bool suspended = false;
    public:
        I2cBusAccessor() {
            /* ... */
        }
    private:
        inline void ClearInterruptMask() const {
            WriteRegister(&i2c_registers->I2C_INTERRUPT_MASK_REGISTER_0, 0);
            ReadRegister(&i2c_registers->I2C_INTERRUPT_MASK_REGISTER_0);
        }

        void SetBus(I2cBus bus);
        void CreateInterruptEvent(I2cBus bus);
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
        void Open(I2cBus bus, SpeedMode speed_mode);
        void Close();
        void Suspend();
        void Resume();
        void DoInitialConfig();

        size_t GetOpenSessions() const;
        bool GetBusy() const;

        void OnStartTransaction() const;
        Result StartTransaction(DriverCommand command, AddressingMode addressing_mode, u32 slave_address);
        Result Send(const u8 *data, size_t num_bytes, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address);
        Result Receive(u8 *out_data, size_t num_bytes, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address);
        void OnStopTransaction() const;
};
