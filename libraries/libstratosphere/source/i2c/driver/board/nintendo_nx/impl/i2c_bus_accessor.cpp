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
#include <stratosphere.hpp>
#include "i2c_bus_accessor.hpp"

namespace ams::i2c::driver::board::nintendo_nx::impl {

    void I2cBusAccessor::Initialize(dd::PhysicalAddress reg_paddr, size_t reg_size, os::InterruptName intr, bool pb, SpeedMode sm) {
        AMS_ASSERT(this->state == State::NotInitialized);

        this->is_power_bus        = pb;
        this->speed_mode          = sm;
        this->interrupt_name      = intr;
        this->registers_phys_addr = reg_paddr;
        this->registers_size      = reg_size;
        this->state               = State::Initializing;
    }

    void I2cBusAccessor::RegisterDeviceCode(DeviceCode dc) {
        AMS_ASSERT(this->state == State::Initializing);

        this->device_code = dc;
    }

    void I2cBusAccessor::InitializeDriver() {
        AMS_ASSERT(this->state == State::Initializing);

        this->registers = reinterpret_cast<volatile I2cRegisters *>(dd::QueryIoMapping(this->registers_phys_addr, this->registers_size));
        AMS_ABORT_UNLESS(this->registers != nullptr);

        this->state = State::Initialized;
    }

    void I2cBusAccessor::FinalizeDriver() {
        AMS_ASSERT(this->state == State::Initialized);
        this->state = State::Initializing;
    }

    Result I2cBusAccessor::InitializeDevice(I2cDeviceProperty *device) {
        /* Check that the device is valid. */
        AMS_ASSERT(device != nullptr);
        AMS_ASSERT(this->state == State::Initialized);

        /* Acquire exclusive access. */
        std::scoped_lock lk(this->user_count_mutex);

        /* Increment our user count -- if we're already open, we're done. */
        AMS_ASSERT(this->user_count >= 0);
        ++this->user_count;
        R_SUCCEED_IF(this->user_count > 1);

        /* Initialize our interrupt event. */
        os::InitializeInterruptEvent(std::addressof(this->interrupt_event), this->interrupt_name, os::EventClearMode_ManualClear);
        os::ClearInterruptEvent(std::addressof(this->interrupt_event);

        /* If we're not power bus, perform power management init. */
        if (!this->is_power_bus) {
            /* Initialize regulator library. */
            regulator::Initialize();

            /* Try to open regulator session. */
            R_TRY(this->TryOpenRegulatorSession());

            /* If we have a regulator session, set voltage to 2.9V. */
            if (this->has_regulator_session) {
                /* NOTE: Nintendo does not check the result, here. */
                regulator::SetVoltageValue(std::addressof(this->regulator_session), 2'900'000u);
            }

            /* Initialize clock/reset library. */
            clkrst::Initialize();
        }

        /* Execute initial config. */
        this->ExecuteInitialConfig();

        /* If we have a regulator session, enable voltage. */
        if (!this->is_power_bus && this->has_regulator_session) {
            /* Check whether voltage was already enabled. */
            const bool was_enabled = regulator::GetVoltageEnabled(std::addressof(this->regulator_session));

            /* NOTE: Nintendo does not check the result of this call. */
            regulator::SetVoltageEnabled(std::addressof(this->regulator_session), true);

            /* If we enabled voltage, delay to give our enable time to take. */
            if (!was_enabled) {
                os::SleepThread(TimeSpan::FromMicroSeconds(560));
            }
        }

        return ResultSuccess();
    }

    void I2cBusAccessor::FinalizeDevice(I2cDeviceProperty *device) {
        /* Check that the device is valid. */
        AMS_ASSERT(device != nullptr);
        AMS_ASSERT(this->state == State::Initialized);

        /* Acquire exclusive access. */
        std::scoped_lock lk(this->user_count_mutex);

        /* Increment our user count -- if we're not the last user, we're done. */
        AMS_ASSERT(this->user_count > 0);
        --this->user_count;
        R_SUCCEED_IF(this->user_count > 0);

        /* Finalize our interrupt event. */
        os::FinalizeInterruptEvent(std::addressof(this->interrupt_event));

        /* If we have a regulator session, disable voltage. */
        if (this->has_regulator_session) {
            /* NOTE: Nintendo does not check the result of this call. */
            regulator::SetVoltageEnabled(std::addressof(this->regulator_session), false);
        }

        /* Finalize the clock/reset library. */
        clkrst::Finalize();

        /* If we have a regulator session, close it. */
        if (this->has_regulator_session) {
            regulator::CloseSession(std::addressof(this->regulator_session));
            this->has_regulator_session = false;
        }

        /* Finalize the regulator library. */
        regulator::Finalize();
    }

    Result I2cBusAccessor::Send(I2cDeviceProperty *device, const void *src, size_t src_size, TransactionOption option) {
        /* TODO */
        AMS_ABORT();
    }

    Result I2cBusAccessor::Receive(void *dst, size_t dst_size, I2cDeviceProperty *device, TransactionOption option) {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::SuspendBus() {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::SuspendPowerBus() {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::ResumeBus() {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::ResumePowerBus() {
        /* TODO */
        AMS_ABORT();
    }

    Result I2cBusAccessor::TryOpenRegulatorSession() {
        /* Ensure we track the session. */
        this->has_regulator_session = true;
        auto s_guard = SCOPE_GUARD { this->has_regulator_session = false; };

        /* Try to open the session. */
        R_TRY_CATCH(regulator::OpenSession(std::addressof(this->regulator_session), this->device_code)) {
            R_CATCH(ddsf::ResultDeviceCodeNotFound) {
                /* It's okay if the device isn't found, but we don't have a session if so. */
                this->has_regulator_session = false;
            }
        } R_END_TRY_CATCH;

        /* We opened (or not). */
        s_guard.Cancel();
        return ResultSuccess();
    }

    void I2cBusAccessor::ExecuteInitialConfig() {
        /* TODO */
        AMS_ABORT();
    }

    Result I2cBusAccessor::Send(const u8 *src, size_t src_size, TransactionOption option, u16 slave_address, AddressingMode addressing_mode) {
        /* TODO */
        AMS_ABORT();
    }

    Result I2cBusAccessor::Receive(u8 *dst, size_t dst_size, TransactionOption option, u16 slave_address, AddressingMode addressing_mode) {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::WriteHeader(Xfer xfer, size_t size, TransactionOption option, u16 slave_address, AddressingMode addressing_mode) {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::ResetController() const {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::ClearBus() const {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::SetClockRegisters(SpeedMode speed_mode) {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::SetPacketModeRegisters() {
        /* TODO */
        AMS_ABORT();
    }

    Result I2cBusAccessor::FlushFifos() {
        /* TODO */
        AMS_ABORT();
    }

    Result I2cBusAccessor::GetTransactionResult() const {
        /* TODO */
        AMS_ABORT();
    }

    void I2cBusAccessor::HandleTransactionError(Result result) {
        /* TODO */
        AMS_ABORT();
    }

}
