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
#include "i2c_pcv.hpp"
#include "i2c_bus_accessor.hpp"

namespace ams::i2c::driver::impl {

    void BusAccessor::Open(Bus bus, SpeedMode speed_mode) {
        std::scoped_lock lk(this->open_mutex);
        /* Open new session. */
        this->open_sessions++;

        /* Ensure we're good if this isn't our first session. */
        if (this->open_sessions > 1) {
            AMS_ABORT_UNLESS(this->speed_mode == speed_mode);
            return;
        }

        /* Set all members for chosen bus. */
        {
            std::scoped_lock lk(this->register_mutex);
            /* Set bus/registers. */
            this->SetBus(bus);
            /* Set pcv module. */
            this->pcv_module = ConvertToPcvModule(bus);
            /* Set speed mode. */
            this->speed_mode = speed_mode;
            /* Setup interrupt event. */
            this->CreateInterruptEvent(bus);
        }
    }

    void BusAccessor::Close() {
        std::scoped_lock lk(this->open_mutex);
        /* Close current session. */
        this->open_sessions--;
        if (this->open_sessions > 0) {
            return;
        }

        /* Close interrupt event. */
        os::FinalizeInterruptEvent(std::addressof(this->interrupt_event));

        /* Close PCV. */
        pcv::Finalize();

        this->suspended = false;
    }

    void BusAccessor::Suspend() {
        std::scoped_lock lk(this->open_mutex);
        std::scoped_lock lk_reg(this->register_mutex);

        if (!this->suspended) {
            this->suspended = true;

            if (this->pcv_module != PcvModule_I2C5) {
                this->DisableClock();
            }
        }
    }

    void BusAccessor::Resume() {
        if (this->suspended) {
            this->DoInitialConfig();
            this->suspended = false;
        }
    }

    void BusAccessor::DoInitialConfig() {
        std::scoped_lock lk(this->register_mutex);

        if (this->pcv_module != PcvModule_I2C5) {
            pcv::Initialize();
        }

        this->ResetController();
        this->SetClock(this->speed_mode);
        this->SetPacketMode();
        this->FlushFifos();
    }

    size_t BusAccessor::GetOpenSessions() const {
        return this->open_sessions;
    }

    bool BusAccessor::GetBusy() const {
        /* Nintendo has a loop here that calls a member function to check if busy, retrying a few times. */
        /* This member function does "return false". */
        /* We will not bother with the loop. */
        return false;
    }

    void BusAccessor::OnStartTransaction() const {
        /* Nothing actually happens here. */
    }

    void BusAccessor::OnStopTransaction() const {
        /* Nothing actually happens here. */
    }

    Result BusAccessor::StartTransaction(Command command, AddressingMode addressing_mode, u32 slave_address) {
        /* Nothing actually happens here... */
        return ResultSuccess();
    }

    Result BusAccessor::Send(const u8 *data, size_t num_bytes, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address) {
        std::scoped_lock lk(this->register_mutex);
        const u8 *cur_src = data;
        size_t remaining = num_bytes;

        /* Set interrupt enable, clear interrupt status. */
        reg::Write(&this->i2c_registers->I2C_INTERRUPT_MASK_REGISTER_0,   0x8E);
        reg::Write(&this->i2c_registers->I2C_INTERRUPT_STATUS_REGISTER_0, 0xFC);

        ON_SCOPE_EXIT { this->ClearInterruptMask(); };

        /* Send header. */
        this->WriteTransferHeader(TransferMode::Send, option, addressing_mode, slave_address, num_bytes);

        /* Send bytes. */
        while (true) {
            const u32 fifo_status = reg::Read(&this->i2c_registers->I2C_FIFO_STATUS_0);
            const size_t fifo_cnt = (fifo_status >> 4);

            for (size_t fifo_idx = 0; remaining > 0 && fifo_idx < fifo_cnt; fifo_idx++) {
                const size_t cur_bytes = std::min(remaining, sizeof(u32));
                u32 val = 0;
                for (size_t i = 0; i < cur_bytes; i++) {
                    val |= cur_src[i] << (8 * i);
                }
                reg::Write(&this->i2c_registers->I2C_I2C_TX_PACKET_FIFO_0, val);

                cur_src += cur_bytes;
                remaining -= cur_bytes;
            }

            if (remaining == 0) {
                break;
            }

            os::ClearInterruptEvent(std::addressof(this->interrupt_event));
            if (!os::TimedWaitInterruptEvent(std::addressof(this->interrupt_event), InterruptTimeout)) {
                this->HandleTransactionResult(i2c::ResultBusBusy());
                os::ClearInterruptEvent(std::addressof(this->interrupt_event));
                return i2c::ResultTimedOut();
            }

            R_TRY(this->GetAndHandleTransactionResult());
        }

        reg::Write(&this->i2c_registers->I2C_INTERRUPT_MASK_REGISTER_0, 0x8C);

        /* Wait for successful completion. */
        while (true) {
            R_TRY(this->GetAndHandleTransactionResult());

            /* Check PACKET_XFER_COMPLETE */
            const u32 interrupt_status = reg::Read(&this->i2c_registers->I2C_INTERRUPT_STATUS_REGISTER_0);
            if (interrupt_status & 0x80) {
                R_TRY(this->GetAndHandleTransactionResult());
                break;
            }

            os::ClearInterruptEvent(std::addressof(this->interrupt_event));
            if (!os::TimedWaitInterruptEvent(std::addressof(this->interrupt_event), InterruptTimeout)) {
                this->HandleTransactionResult(i2c::ResultBusBusy());
                os::ClearInterruptEvent(std::addressof(this->interrupt_event));
                return i2c::ResultTimedOut();
            }
        }

        return ResultSuccess();
    }

    Result BusAccessor::Receive(u8 *out_data, size_t num_bytes, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address) {
        std::scoped_lock lk(this->register_mutex);
        u8 *cur_dst = out_data;
        size_t remaining = num_bytes;

        /* Set interrupt enable, clear interrupt status. */
        reg::Write(&this->i2c_registers->I2C_INTERRUPT_MASK_REGISTER_0,   0x8D);
        reg::Write(&this->i2c_registers->I2C_INTERRUPT_STATUS_REGISTER_0, 0xFC);

        /* Send header. */
        this->WriteTransferHeader(TransferMode::Receive, option, addressing_mode, slave_address, num_bytes);

        /* Receive bytes. */
        while (remaining > 0) {
            os::ClearInterruptEvent(std::addressof(this->interrupt_event));
            if (!os::TimedWaitInterruptEvent(std::addressof(this->interrupt_event), InterruptTimeout)) {
                this->HandleTransactionResult(i2c::ResultBusBusy());
                this->ClearInterruptMask();
                os::ClearInterruptEvent(std::addressof(this->interrupt_event));
                return i2c::ResultTimedOut();
            }

            R_TRY(this->GetAndHandleTransactionResult());

            const u32 fifo_status = reg::Read(&this->i2c_registers->I2C_FIFO_STATUS_0);
            const size_t fifo_cnt = std::min((remaining + 3) >> 2, static_cast<size_t>(fifo_status & 0xF));

            for (size_t fifo_idx = 0; remaining > 0 && fifo_idx < fifo_cnt; fifo_idx++) {
                const u32 val = reg::Read(&this->i2c_registers->I2C_I2C_RX_FIFO_0);
                const size_t cur_bytes = std::min(remaining, sizeof(u32));
                for (size_t i = 0; i < cur_bytes; i++) {
                    cur_dst[i] = static_cast<u8>((val >> (8 * i)) & 0xFF);
                }

                cur_dst += cur_bytes;
                remaining -= cur_bytes;
            }
        }

        /* N doesn't do ClearInterruptMask. */
        return ResultSuccess();
    }

    void BusAccessor::SetBus(Bus bus) {
        this->bus = bus;
        this->i2c_registers = GetRegisters(bus);
        this->clkrst_registers.SetBus(bus);
    }

    void BusAccessor::CreateInterruptEvent(Bus bus) {
        static constexpr u64 s_interrupts[] = {
            0x46, 0x74, 0x7C, 0x98, 0x55, 0x5F
        };
        const auto index = ConvertToIndex(bus);
        AMS_ABORT_UNLESS(index < util::size(s_interrupts));
        os::InitializeInterruptEvent(std::addressof(this->interrupt_event), s_interrupts[index], os::EventClearMode_ManualClear);
        os::ClearInterruptEvent(std::addressof(this->interrupt_event));
    }

    void BusAccessor::SetClock(SpeedMode speed_mode) {
        u32 t_high, t_low;
        u32 clk_div, src_div;
        u32 debounce;

        switch (speed_mode) {
            case SpeedMode::Normal:
                t_high = 2;
                t_low = 4;
                clk_div = 0x19;
                src_div = 0x13;
                debounce = 2;
                break;
            case SpeedMode::Fast:
                t_high = 2;
                t_low = 4;
                clk_div = 0x19;
                src_div = 0x04;
                debounce = 2;
                break;
            case SpeedMode::FastPlus:
                t_high = 2;
                t_low = 4;
                clk_div = 0x10;
                src_div = 0x02;
                debounce = 0;
                break;
            case SpeedMode::HighSpeed:
                t_high = 3;
                t_low = 8;
                clk_div = 0x02;
                src_div = 0x02;
                debounce = 0;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        if (speed_mode == SpeedMode::HighSpeed) {
            reg::Write(&this->i2c_registers->I2C_I2C_HS_INTERFACE_TIMING_0_0, (t_high << 8) | (t_low));
            reg::Write(&this->i2c_registers->I2C_I2C_CLK_DIVISOR_REGISTER_0, clk_div);
        } else {
            reg::Write(&this->i2c_registers->I2C_I2C_INTERFACE_TIMING_0_0, (t_high << 8) | (t_low));
            reg::Write(&this->i2c_registers->I2C_I2C_CLK_DIVISOR_REGISTER_0, (clk_div << 16));
        }

        reg::Write(&this->i2c_registers->I2C_I2C_CNFG_0, debounce);
        reg::Read(&this->i2c_registers->I2C_I2C_CNFG_0);

        if (this->pcv_module != PcvModule_I2C5) {
            R_ABORT_UNLESS(pcv::SetReset(this->pcv_module, true));
            R_ABORT_UNLESS(pcv::SetClockRate(this->pcv_module, (408'000'000) / (src_div + 1)));
            R_ABORT_UNLESS(pcv::SetReset(this->pcv_module, false));
        }
    }

    void BusAccessor::ResetController() const {
        if (this->pcv_module != PcvModule_I2C5) {
            R_ABORT_UNLESS(pcv::SetReset(this->pcv_module, true));
            R_ABORT_UNLESS(pcv::SetClockRate(this->pcv_module, 81'600'000));
            R_ABORT_UNLESS(pcv::SetReset(this->pcv_module, false));
        }
    }

    void BusAccessor::ClearBus() const {
        bool success = false;
        for (size_t i = 0; i < 3 && !success; i++) {
            success = true;

            this->ResetController();

            reg::Write(&this->i2c_registers->I2C_I2C_BUS_CLEAR_CONFIG_0, 0x90000);
            reg::SetBits(&this->i2c_registers->I2C_I2C_BUS_CLEAR_CONFIG_0, 0x4);
            reg::SetBits(&this->i2c_registers->I2C_I2C_BUS_CLEAR_CONFIG_0, 0x2);

            reg::SetBits(&this->i2c_registers->I2C_I2C_CONFIG_LOAD_0, 0x1);
            {
                u64 start_tick = armGetSystemTick();
                while (reg::Read(&this->i2c_registers->I2C_I2C_CONFIG_LOAD_0) & 1) {
                    if (armTicksToNs(armGetSystemTick() - start_tick) > 1'000'000) {
                        success = false;
                        break;
                    }
                }
            }
            if (!success) {
                continue;
            }

            reg::SetBits(&this->i2c_registers->I2C_I2C_BUS_CLEAR_CONFIG_0, 0x1);
            {
                u64 start_tick = armGetSystemTick();
                while (reg::Read(&this->i2c_registers->I2C_I2C_BUS_CLEAR_CONFIG_0) & 1) {
                    if (armTicksToNs(armGetSystemTick() - start_tick) > 1'000'000) {
                        success = false;
                        break;
                    }
                }
            }
            if (!success) {
                continue;
            }

            {
                u64 start_tick = armGetSystemTick();
                while (reg::Read(&this->i2c_registers->I2C_I2C_BUS_CLEAR_STATUS_0) & 1) {
                    if (armTicksToNs(armGetSystemTick() - start_tick) > 1'000'000) {
                        success = false;
                        break;
                    }
                }
            }
            if (!success) {
                continue;
            }
        }
    }

    void BusAccessor::DisableClock() {
        R_ABORT_UNLESS(pcv::SetClockEnabled(this->pcv_module, false));
    }

    void BusAccessor::SetPacketMode() {
        /* Set PACKET_MODE_EN, MSTR_CONFIG_LOAD */
        reg::SetBits(&this->i2c_registers->I2C_I2C_CNFG_0, 0x400);
        reg::SetBits(&this->i2c_registers->I2C_I2C_CONFIG_LOAD_0, 0x1);

        /* Set TX_FIFO_TRIGGER, RX_FIFO_TRIGGER */
        reg::Write(&this->i2c_registers->I2C_FIFO_CONTROL_0, 0xFC);
    }

    Result BusAccessor::FlushFifos() {
        reg::Write(&this->i2c_registers->I2C_FIFO_CONTROL_0, 0xFF);

        /* Wait for flush to finish, check every ms for 5 ms. */
        for (size_t i = 0; i < 5; i++) {
            const bool flush_done = (reg::Read(&this->i2c_registers->I2C_FIFO_CONTROL_0) & 3) == 0;
            R_SUCCEED_IF(flush_done);
            svcSleepThread(1'000'000ul);
        }

        return i2c::ResultBusBusy();
    }

    Result BusAccessor::GetTransactionResult() const {
        const u32 packet_status = reg::Read(&this->i2c_registers->I2C_PACKET_TRANSFER_STATUS_0);
        const u32 interrupt_status = reg::Read(&this->i2c_registers->I2C_INTERRUPT_STATUS_REGISTER_0);

        /* Check for no ack. */
        R_UNLESS(!(packet_status & 0xC),    i2c::ResultNoAck());
        R_UNLESS(!(interrupt_status & 0x8), i2c::ResultNoAck());

        /* Check for arb lost. */
        {
            auto bus_guard = SCOPE_GUARD { this->ClearBus(); };
            R_UNLESS(!(packet_status & 0x2),    i2c::ResultBusBusy());
            R_UNLESS(!(interrupt_status & 0x4), i2c::ResultBusBusy());
            bus_guard.Cancel();
        }

        return ResultSuccess();
    }

    void BusAccessor::HandleTransactionResult(Result result) {
        R_TRY_CATCH(result) {
            R_CATCH(i2c::ResultNoAck, i2c::ResultBusBusy) {
                this->ResetController();
                this->SetClock(this->speed_mode);
                this->SetPacketMode();
                this->FlushFifos();
            }
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
    }

    Result BusAccessor::GetAndHandleTransactionResult() {
        const auto transaction_result = this->GetTransactionResult();
        R_SUCCEED_IF(R_SUCCEEDED(transaction_result));

        this->HandleTransactionResult(transaction_result);
        this->ClearInterruptMask();
        os::ClearInterruptEvent(std::addressof(this->interrupt_event));
        return transaction_result;
    }

    void BusAccessor::WriteTransferHeader(TransferMode transfer_mode, I2cTransactionOption option, AddressingMode addressing_mode, u32 slave_address, size_t num_bytes) {
        this->FlushFifos();

        reg::Write(&this->i2c_registers->I2C_I2C_TX_PACKET_FIFO_0, 0x10);
        reg::Write(&this->i2c_registers->I2C_I2C_TX_PACKET_FIFO_0, static_cast<u32>(num_bytes - 1) & 0xFFF);

        const u32 slave_addr_val = ((transfer_mode == TransferMode::Receive) & 1) | ((slave_address & 0x7F) << 1);
        u32 hdr_val = 0;
        hdr_val |= ((this->speed_mode == SpeedMode::HighSpeed) & 1) << 22;
        hdr_val |= ((transfer_mode == TransferMode::Receive) & 1) << 19;
        hdr_val |= ((addressing_mode != AddressingMode::SevenBit) & 1) << 18;
        hdr_val |= (1 << 17);
        hdr_val |= (((option & I2cTransactionOption_Stop) == 0) & 1) << 16;
        hdr_val |= slave_addr_val;

        reg::Write(&this->i2c_registers->I2C_I2C_TX_PACKET_FIFO_0, hdr_val);
    }

}
