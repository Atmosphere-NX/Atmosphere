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
#include "i2c_session.hpp"

namespace ams::i2c::driver::impl {

    void Session::Open(Bus bus, u32 slave_address, AddressingMode addr_mode, SpeedMode speed_mode, BusAccessor *bus_accessor, u32 max_retries, u64 retry_wait_time) {
        std::scoped_lock lk(this->bus_accessor_mutex);
        if (!this->open) {
            this->bus_accessor = bus_accessor;
            this->bus = bus;
            this->slave_address = slave_address;
            this->addressing_mode = addr_mode;
            this->max_retries = max_retries;
            this->retry_wait_time = retry_wait_time;
            this->bus_accessor->Open(this->bus, speed_mode);
            this->open = true;
        }
    }

    void Session::Start() {
        std::scoped_lock lk(this->bus_accessor_mutex);
        if (this->open) {
            if (this->bus_accessor->GetOpenSessions() == 1) {
                this->bus_accessor->DoInitialConfig();
            }
        }
    }

    void Session::Close() {
        std::scoped_lock lk(this->bus_accessor_mutex);
        if (this->open) {
            this->bus_accessor->Close();
            this->bus_accessor = nullptr;
            this->open = false;
        }
    }

    bool Session::IsOpen() const {
        return this->open;
    }

    Result Session::DoTransaction(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, Command command) {
        std::scoped_lock lk(this->bus_accessor_mutex);

        R_UNLESS(!this->bus_accessor->GetBusy(), i2c::ResultBusBusy());

        this->bus_accessor->OnStartTransaction();
        ON_SCOPE_EXIT { this->bus_accessor->OnStopTransaction(); };

        R_TRY(this->bus_accessor->StartTransaction(command, this->addressing_mode, this->slave_address));

        switch (command) {
            case Command::Send:
                R_TRY(this->bus_accessor->Send(reinterpret_cast<const u8 *>(src), num_bytes, option, this->addressing_mode, this->slave_address));
                break;
            case Command::Receive:
                R_TRY(this->bus_accessor->Receive(reinterpret_cast<u8 *>(dst), num_bytes, option, this->addressing_mode, this->slave_address));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return ResultSuccess();
    }

    Result Session::DoTransactionWithRetry(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, Command command) {
        size_t i = 0;
        while (true) {
            R_TRY_CATCH(this->DoTransaction(dst, src, num_bytes, option, command)) {
                R_CATCH(i2c::ResultTimedOut) {
                    if ((++i) <= this->max_retries) {
                        svcSleepThread(this->retry_wait_time);
                        continue;
                    }
                    return i2c::ResultBusBusy();
                }
            } R_END_TRY_CATCH;
            return ResultSuccess();
        }
    }

}
