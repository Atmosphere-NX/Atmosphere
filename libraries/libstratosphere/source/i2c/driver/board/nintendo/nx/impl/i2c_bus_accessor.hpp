/*
 * Copyright (c) Atmosphère-NX
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
            volatile I2cRegisters *m_registers;
            SpeedMode m_speed_mode;
            os::InterruptEventType m_interrupt_event;
            int m_user_count;
            os::SdkMutex m_user_count_mutex;
            os::SdkMutex m_register_mutex;
            regulator::RegulatorSession m_regulator_session;
            bool m_has_regulator_session;
            State m_state;
            os::SdkMutex m_transaction_order_mutex;
            bool m_is_power_bus;
            dd::PhysicalAddress m_registers_phys_addr;
            size_t m_registers_size;
            os::InterruptName m_interrupt_name;
            DeviceCode m_device_code;
            util::IntrusiveListNode m_bus_accessor_list_node;
        public:
            using BusAccessorListTraits = util::IntrusiveListMemberTraits<&I2cBusAccessor::m_bus_accessor_list_node>;
            using BusAccessorList       = typename BusAccessorListTraits::ListType;
            friend class util::IntrusiveList<I2cBusAccessor, util::IntrusiveListMemberTraits<&I2cBusAccessor::m_bus_accessor_list_node>>;
        public:
            I2cBusAccessor()
                : m_registers(nullptr), m_speed_mode(SpeedMode_Fast), m_user_count(0), m_user_count_mutex(),
                 m_register_mutex(), m_has_regulator_session(false), m_state(State::NotInitialized), m_transaction_order_mutex(),
                 m_is_power_bus(false), m_registers_phys_addr(0), m_registers_size(0), m_interrupt_name(), m_device_code(-1), m_bus_accessor_list_node()
            {
                /* ... */
            }

            void Initialize(dd::PhysicalAddress reg_paddr, size_t reg_size, os::InterruptName intr, bool pb, SpeedMode sm);
            void RegisterDeviceCode(DeviceCode device_code);

            SpeedMode GetSpeedMode() const { return m_speed_mode; }
            dd::PhysicalAddress GetRegistersPhysicalAddress() const { return m_registers_phys_addr; }
            size_t GetRegistersSize() const { return m_registers_size; }
            os::InterruptName GetInterruptName() const { return m_interrupt_name; }
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
                reg::Write(m_registers->interrupt_mask_register, 0);
                reg::Read(m_registers->interrupt_mask_register);
            }

            Result CheckAndHandleError() {
                const Result result = this->GetTransactionResult();
                this->HandleTransactionError(result);
                if (R_FAILED(result)) {
                    this->DisableInterruptMask();
                    os::ClearInterruptEvent(std::addressof(m_interrupt_event));
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
                return m_transaction_order_mutex;
            }

            virtual void SuspendBus() override;
            virtual void SuspendPowerBus() override;

            virtual void ResumeBus() override;
            virtual void ResumePowerBus() override;

            virtual const DeviceCode &GetDeviceCode() const override {
                return m_device_code;
            }
    };

}
