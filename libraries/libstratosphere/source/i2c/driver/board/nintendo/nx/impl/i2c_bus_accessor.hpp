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
#include <stratosphere.hpp>
#include "i2c_i2c_registers.hpp"

namespace ams::i2c::driver::board::nintendo::nx::impl {

    class I2cBusAccessor : public ::ams::i2c::driver::II2cDriver {
        NON_COPYABLE(I2cBusAccessor);
        NON_MOVEABLE(I2cBusAccessor);
        AMS_DDSF_CASTABLE_TRAITS(ams::i2c::driver::board::nintendo::nx::impl::I2cBusAccessor, ::ams::i2c::driver::II2cDriver);
        private:
            enum class State {
                NotInitialized    = 0,
                Initializing      = 1,
                Initialized       = 2,
                Suspended         = 3,
                PowerBusSuspended = 4,
            };

            enum Xfer {
                Xfer_Write = 0,
                Xfer_Read  = 1,
            };
        private:
            volatile I2cRegisters *registers;
            SpeedMode speed_mode;
            os::InterruptEventType interrupt_event;
            int user_count;
            os::SdkMutex user_count_mutex;
            os::SdkMutex register_mutex;
            regulator::RegulatorSession regulator_session;
            bool has_regulator_session;
            State state;
            os::SdkMutex transaction_order_mutex;
            bool is_power_bus;
            dd::PhysicalAddress registers_phys_addr;
            size_t registers_size;
            os::InterruptName interrupt_name;
            DeviceCode device_code;
            util::IntrusiveListNode bus_accessor_list_node;
        public:
            using BusAccessorListTraits = util::IntrusiveListMemberTraitsDeferredAssert<&I2cBusAccessor::bus_accessor_list_node>;
            using BusAccessorList       = typename BusAccessorListTraits::ListType;
            friend class util::IntrusiveList<I2cBusAccessor, util::IntrusiveListMemberTraitsDeferredAssert<&I2cBusAccessor::bus_accessor_list_node>>;
        public:
            I2cBusAccessor()
                : registers(nullptr), speed_mode(SpeedMode_Fast), user_count(0), user_count_mutex(),
                 register_mutex(), has_regulator_session(false), state(State::NotInitialized), transaction_order_mutex(),
                 is_power_bus(false), registers_phys_addr(0), registers_size(0), interrupt_name(), device_code(-1), bus_accessor_list_node()
            {
                /* ... */
            }

            void Initialize(dd::PhysicalAddress reg_paddr, size_t reg_size, os::InterruptName intr, bool pb, SpeedMode sm);
            void RegisterDeviceCode(DeviceCode device_code);

            SpeedMode GetSpeedMode() const { return this->speed_mode; }
            dd::PhysicalAddress GetRegistersPhysicalAddress() const { return this->registers_phys_addr; }
            size_t GetRegistersSize() const { return this->registers_size; }
            os::InterruptName GetInterruptName() const { return this->interrupt_name; }
        private:
            Result TryOpenRegulatorSession();

            void ExecuteInitialConfig();

            Result Send(const u8 *src, size_t src_size, TransactionOption option, u16 slave_address, AddressingMode addressing_mode);
            Result Receive(u8 *dst, size_t dst_size, TransactionOption option, u16 slave_address, AddressingMode addressing_mode);

            void WriteHeader(Xfer xfer, size_t size, TransactionOption option, u16 slave_address, AddressingMode addressing_mode);

            void ResetController() const;
            void ClearBus() const;
            void SetClockRegisters(SpeedMode speed_mode);
            void SetPacketModeRegisters();

            Result FlushFifos();

            Result GetTransactionResult() const;
            void HandleTransactionError(Result result);

            void DisableInterruptMask() {
                reg::Write(this->registers->interrupt_mask_register, 0);
                reg::Read(this->registers->interrupt_mask_register);
            }

            Result CheckAndHandleError() {
                const Result result = this->GetTransactionResult();
                this->HandleTransactionError(result);
                if (R_FAILED(result)) {
                    this->DisableInterruptMask();
                    os::ClearInterruptEvent(std::addressof(this->interrupt_event));
                    return result;
                }

                return ResultSuccess();
            }
        public:
            virtual void InitializeDriver() override;
            virtual void FinalizeDriver() override;

            virtual Result InitializeDevice(I2cDeviceProperty *device) override;
            virtual void FinalizeDevice(I2cDeviceProperty *device) override;

            virtual Result Send(I2cDeviceProperty *device, const void *src, size_t src_size, TransactionOption option) override;
            virtual Result Receive(void *dst, size_t dst_size, I2cDeviceProperty *device, TransactionOption option) override;

            virtual os::SdkMutex &GetTransactionOrderMutex() override {
                return this->transaction_order_mutex;
            }

            virtual void SuspendBus() override;
            virtual void SuspendPowerBus() override;

            virtual void ResumeBus() override;
            virtual void ResumePowerBus() override;

            virtual const DeviceCode &GetDeviceCode() const override {
                return this->device_code;
            }
    };

}
