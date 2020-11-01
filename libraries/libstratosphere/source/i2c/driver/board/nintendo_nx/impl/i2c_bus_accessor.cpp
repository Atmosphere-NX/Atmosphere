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
        os::ClearInterruptEvent(std::addressof(this->interrupt_event));

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
        if (this->user_count > 0) {
            return;
        }

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
        /* Check pre-conditions. */
        AMS_ASSERT(device != nullptr);
        AMS_ASSERT(src != nullptr);
        AMS_ASSERT(src_size > 0);

        if (this->is_power_bus) {
            AMS_ASSERT(this->state == State::Initialized || this->state == State::Suspended);
        } else {
            AMS_ASSERT(this->state == State::Initialized);
        }

        /* Send the data. */
        return this->Send(static_cast<const u8 *>(src), src_size, option, device->GetAddress(), device->GetAddressingMode());
    }

    Result I2cBusAccessor::Receive(void *dst, size_t dst_size, I2cDeviceProperty *device, TransactionOption option) {
        /* Check pre-conditions. */
        AMS_ASSERT(device != nullptr);
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size > 0);

        if (this->is_power_bus) {
            AMS_ASSERT(this->state == State::Initialized || this->state == State::Suspended);
        } else {
            AMS_ASSERT(this->state == State::Initialized);
        }

        /* Send the data. */
        return this->Receive(static_cast<u8 *>(dst), dst_size, option, device->GetAddress(), device->GetAddressingMode());
    }

    void I2cBusAccessor::SuspendBus() {
        /* Check that state is valid. */
        AMS_ASSERT(this->state == State::Initialized);

        /* Acquire exclusive access. */
        std::scoped_lock lk(this->user_count_mutex);

        /* If we need to, disable clock/voltage appropriately. */
        if (!this->is_power_bus && this->user_count > 0) {
            /* Disable clock. */
            {
                /* Open a clkrst session. */
                clkrst::ClkRstSession clkrst_session;
                R_ABORT_UNLESS(clkrst::OpenSession(std::addressof(clkrst_session), this->device_code));
                ON_SCOPE_EXIT { clkrst::CloseSession(std::addressof(clkrst_session)); };

                /* Set clock disabled for the session. */
                clkrst::SetClockDisabled(std::addressof(clkrst_session));
            }

            /* Disable voltage. */
            if (this->has_regulator_session) {
                regulator::SetVoltageEnabled(std::addressof(this->regulator_session), false);
            }
        }

        /* Update state. */
        this->state = State::Suspended;
    }

    void I2cBusAccessor::SuspendPowerBus() {
        /* Check that state is valid. */
        AMS_ASSERT(this->state == State::Suspended);

        /* Acquire exclusive access. */
        std::scoped_lock lk(this->user_count_mutex);

        /* If we need to, disable clock/voltage appropriately. */
        if (this->is_power_bus && this->user_count > 0) {
            /* Nothing should actually be done here. */
        }

        /* Update state. */
        this->state = State::PowerBusSuspended;
    }

    void I2cBusAccessor::ResumeBus() {
        /* Check that state is valid. */
        AMS_ASSERT(this->state == State::Suspended);

        /* Acquire exclusive access. */
        std::scoped_lock lk(this->user_count_mutex);

        /* If we need to, enable clock/voltage appropriately. */
        if (!this->is_power_bus && this->user_count > 0) {
            /* Enable voltage. */
            if (this->has_regulator_session) {
                /* Check whether voltage was already enabled. */
                const bool was_enabled = regulator::GetVoltageEnabled(std::addressof(this->regulator_session));

                /* NOTE: Nintendo does not check the result of this call. */
                regulator::SetVoltageEnabled(std::addressof(this->regulator_session), true);

                /* If we enabled voltage, delay to give our enable time to take. */
                if (!was_enabled) {
                    os::SleepThread(TimeSpan::FromMicroSeconds(560));
                }
            }

            /* Execute initial config, which will enable clock as relevant. */
            this->ExecuteInitialConfig();
        }

        /* Update state. */
        this->state = State::Initialized;
    }

    void I2cBusAccessor::ResumePowerBus() {
        /* Check that state is valid. */
        AMS_ASSERT(this->state == State::PowerBusSuspended);

        /* Acquire exclusive access. */
        std::scoped_lock lk(this->user_count_mutex);

        /* If we need to, enable clock/voltage appropriately. */
        if (this->is_power_bus && this->user_count > 0) {
            /* Execute initial config, which will enable clock as relevant. */
            this->ExecuteInitialConfig();
        }

        /* Update state. */
        this->state = State::Suspended;
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
        /* Lock exclusive access to registers. */
        std::scoped_lock lk(this->register_mutex);

        /* Reset the controller. */
        this->ResetController();

        /* Set clock registers. */
        this->SetClockRegisters(this->speed_mode);

        /* Set packet mode registers. */
        this->SetPacketModeRegisters();

        /* Flush fifos. */
        this->FlushFifos();
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
        /* Reset the controller. */
        if (!this->is_power_bus) {
            /* Open a clkrst session. */
            clkrst::ClkRstSession clkrst_session;
            R_ABORT_UNLESS(clkrst::OpenSession(std::addressof(clkrst_session), this->device_code));
            ON_SCOPE_EXIT { clkrst::CloseSession(std::addressof(clkrst_session)); };

            /* Reset the controller, setting clock rate to 408 MHz / 5 (to account for clock divisor). */
            /* NOTE: Nintendo does not check result for any of these calls. */
            clkrst::SetResetAsserted(std::addressof(clkrst_session));
            clkrst::SetClockRate(std::addressof(clkrst_session), 408'000'000 / (4 + 1));
            clkrst::SetResetDeasserted(std::addressof(clkrst_session));
        }
    }

    void I2cBusAccessor::ClearBus() const {
        /* Try to clear the bus up to three times. */
        constexpr int MaxRetryCount        = 3;
        constexpr int BusyLoopMicroSeconds = 1000;

        int try_count = 0;
        bool need_retry;
        do {
            /* Update trackers. */
            ++try_count;
            need_retry = false;

            /* Reset the controller. */
            this->ResetController();

            /* Configure the sclk threshold for bus clear config. */
            reg::Write(this->registers->bus_clear_config, I2C_REG_BITS_VALUE(BUS_CLEAR_CONFIG_BC_SCLK_THRESHOLD, 9));

            /* Set stop cond and terminate in bus clear config. */
            reg::ReadWrite(this->registers->bus_clear_config, I2C_REG_BITS_ENUM(BUS_CLEAR_CONFIG_BC_STOP_COND, STOP));
            reg::ReadWrite(this->registers->bus_clear_config, I2C_REG_BITS_ENUM(BUS_CLEAR_CONFIG_BC_TERMINATE, IMMEDIATE));

            /* Set master config load, busy loop up to 1ms for it to take. */
            reg::ReadWrite(this->registers->config_load, I2C_REG_BITS_ENUM(CONFIG_LOAD_MSTR_CONFIG_LOAD, ENABLE));

            const os::Tick start_tick_a = os::GetSystemTick();
            while (reg::HasValue(this->registers->config_load, I2C_REG_BITS_ENUM(CONFIG_LOAD_MSTR_CONFIG_LOAD, ENABLE))) {
                if ((os::GetSystemTick() - start_tick_a).ToTimeSpan().GetMicroSeconds() > BusyLoopMicroSeconds) {
                    need_retry = true;
                    break;
                }
            }

            if (need_retry) {
                continue;
            }

            /* Set bus clear enable, wait up to 1ms for it to take. */
            reg::ReadWrite(this->registers->bus_clear_config, I2C_REG_BITS_ENUM(BUS_CLEAR_CONFIG_BC_ENABLE, ENABLE));

            const os::Tick start_tick_b = os::GetSystemTick();
            while (reg::HasValue(this->registers->bus_clear_config, I2C_REG_BITS_ENUM(BUS_CLEAR_CONFIG_BC_ENABLE, ENABLE))) {
                if ((os::GetSystemTick() - start_tick_b).ToTimeSpan().GetMicroSeconds() > BusyLoopMicroSeconds) {
                    need_retry = true;
                    break;
                }
            }

            if (need_retry) {
                continue;
            }

            /* Wait up to 1ms for the bus clear to complete. */
            const os::Tick start_tick_c = os::GetSystemTick();
            while (reg::HasValue(this->registers->bus_clear_status, I2C_REG_BITS_ENUM(BUS_CLEAR_STATUS_BC_STATUS, NOT_CLEARED))) {
                if ((os::GetSystemTick() - start_tick_c).ToTimeSpan().GetMicroSeconds() > BusyLoopMicroSeconds) {
                    need_retry = true;
                    break;
                }
            }

            if (need_retry) {
                continue;
            }
        } while (try_count < MaxRetryCount && need_retry);
    }

    void I2cBusAccessor::SetClockRegisters(SpeedMode speed_mode) {
        /* Determine parameters for the speed mode. */
        u32 t_high, t_low, clk_div, debounce, src_div;
        bool high_speed = false;

        if (this->is_power_bus) {
            t_high   = 0x02;
            t_low    = 0x04;
            clk_div  = 0x05;
            debounce = 0x02;
        } else {
            switch (speed_mode) {
                case SpeedMode_Standard:
                    t_high   = 0x02;
                    t_low    = 0x04;
                    clk_div  = 0x19;
                    debounce = 0x02;
                    src_div  = 0x13;
                    break;
                case SpeedMode_Fast:
                    t_high   = 0x02;
                    t_low    = 0x04;
                    clk_div  = 0x19;
                    debounce = 0x02;
                    src_div  = 0x04;
                    break;
                case SpeedMode_FastPlus:
                    t_high   = 0x02;
                    t_low    = 0x04;
                    clk_div  = 0x10;
                    debounce = 0x00;
                    src_div  = 0x02;
                    break;
                case SpeedMode_HighSpeed:
                    t_high   = 0x03;
                    t_low    = 0x08;
                    clk_div  = 0x02;
                    debounce = 0x00;
                    src_div  = 0x02;
                    high_speed = true;
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        /* Write the clock divisors. */
        if (high_speed) {
            reg::Write(this->registers->hs_interface_timing_0, I2C_REG_BITS_VALUE(HS_INTERFACE_TIMING_0_HS_THIGH, t_high),
                                                               I2C_REG_BITS_VALUE(HS_INTERFACE_TIMING_0_HS_TLOW,   t_low));

            reg::Write(this->registers->clk_divisor_register, I2C_REG_BITS_VALUE(CLK_DIVISOR_REGISTER_HSMODE, clk_div));
        } else {
            reg::Write(this->registers->interface_timing_0, I2C_REG_BITS_VALUE(INTERFACE_TIMING_0_THIGH, t_high),
                                                            I2C_REG_BITS_VALUE(INTERFACE_TIMING_0_TLOW,   t_low));

            reg::Write(this->registers->clk_divisor_register, I2C_REG_BITS_VALUE(CLK_DIVISOR_REGISTER_STD_FAST_MODE, clk_div));
        }

        /* Configure debounce. */
        reg::Write(this->registers->cnfg, I2C_REG_BITS_VALUE(I2C_CNFG_DEBOUNCE_CNT, debounce));
        reg::Read(this->registers->cnfg);

        /* Set the clock rate, if we should. */
        if (!this->is_power_bus) {
            /* Open a clkrst session. */
            clkrst::ClkRstSession clkrst_session;
            R_ABORT_UNLESS(clkrst::OpenSession(std::addressof(clkrst_session), this->device_code));
            ON_SCOPE_EXIT { clkrst::CloseSession(std::addressof(clkrst_session)); };

            /* Reset the controller, setting clock rate to 408 MHz / (src_div + 1). */
            /* NOTE: Nintendo does not check result for any of these calls. */
            clkrst::SetResetAsserted(std::addressof(clkrst_session));
            clkrst::SetClockRate(std::addressof(clkrst_session), 408'000'000 / (src_div + 1));
            clkrst::SetResetDeasserted(std::addressof(clkrst_session));
        }
    }

    void I2cBusAccessor::SetPacketModeRegisters() {
        /* Set packet mode enable. */
        reg::ReadWrite(this->registers->cnfg, I2C_REG_BITS_ENUM(I2C_CNFG_PACKET_MODE_EN, GO));

        /* Set master config load. */
        reg::ReadWrite(this->registers->config_load, I2C_REG_BITS_ENUM(CONFIG_LOAD_MSTR_CONFIG_LOAD, ENABLE));

        /* Set tx/fifo triggers to default (maximum values). */
        reg::Write(this->registers->fifo_control, I2C_REG_BITS_VALUE(FIFO_CONTROL_RX_FIFO_TRIG, 7),
                                                  I2C_REG_BITS_VALUE(FIFO_CONTROL_TX_FIFO_TRIG, 7));
    }

    Result I2cBusAccessor::FlushFifos() {
        /* Flush the fifo. */
        reg::Write(this->registers->fifo_control, I2C_REG_BITS_VALUE(FIFO_CONTROL_RX_FIFO_TRIG,    7),
                                                  I2C_REG_BITS_VALUE(FIFO_CONTROL_TX_FIFO_TRIG,    7),
                                                  I2C_REG_BITS_ENUM (FIFO_CONTROL_RX_FIFO_FLUSH, SET),
                                                  I2C_REG_BITS_ENUM (FIFO_CONTROL_TX_FIFO_FLUSH, SET));

        /* Wait up to 5 ms for the flush to complete. */
        int count = 0;
        while (!reg::HasValue(this->registers->fifo_control, I2C_REG_BITS_ENUM(FIFO_CONTROL_FIFO_FLUSH, RX_UNSET_TX_UNSET))) {
            R_UNLESS((++count < 5), i2c::ResultBusBusy());
            os::SleepThread(TimeSpan::FromMilliSeconds(1));
        }

        return ResultSuccess();
    }

    Result I2cBusAccessor::GetTransactionResult() const {
        /* Get packet status/interrupt status. */
        volatile u32 packet_status    = reg::Read(this->registers->packet_transfer_status);
        volatile u32 interrupt_status = reg::Read(this->registers->interrupt_status_register);

        /* Check for ack. */
        R_UNLESS(reg::HasValue(packet_status, I2C_REG_BITS_ENUM(PACKET_TRANSFER_STATUS_NOACK_FOR_DATA, UNSET)), i2c::ResultNoAck());
        R_UNLESS(reg::HasValue(packet_status, I2C_REG_BITS_ENUM(PACKET_TRANSFER_STATUS_NOACK_FOR_ADDR, UNSET)), i2c::ResultNoAck());
        R_UNLESS(reg::HasValue(interrupt_status, I2C_REG_BITS_ENUM(INTERRUPT_STATUS_REGISTER_NOACK,    UNSET)), i2c::ResultNoAck());

        /* If we lost arbitration, we'll need to clear the bus. */
        auto clear_guard = SCOPE_GUARD { this->ClearBus(); };

        /* Check for arb lost. */
        R_UNLESS(reg::HasValue(packet_status, I2C_REG_BITS_ENUM(PACKET_TRANSFER_STATUS_ARB_LOST,       UNSET)), i2c::ResultBusBusy());
        R_UNLESS(reg::HasValue(interrupt_status, I2C_REG_BITS_ENUM(INTERRUPT_STATUS_REGISTER_ARB_LOST, UNSET)), i2c::ResultBusBusy());

        clear_guard.Cancel();
        return ResultSuccess();
    }

    void I2cBusAccessor::HandleTransactionError(Result result) {
        R_TRY_CATCH(result) {
            R_CATCH(i2c::ResultNoAck, i2c::ResultBusBusy) {
                /* Reset the controller. */
                this->ResetController();

                /* Set clock registers. */
                this->SetClockRegisters(this->speed_mode);

                /* Set packet mode registers. */
                this->SetPacketModeRegisters();

                /* Flush fifos. */
                this->FlushFifos();
            }
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
    }

}
