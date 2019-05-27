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

#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_driver_session.hpp"

void I2cDriverSession::Open(I2cBus bus, u32 slave_address, AddressingMode addr_mode, SpeedMode speed_mode, I2cBusAccessor *bus_accessor, u32 max_retries, u64 retry_wait_time){
    std::scoped_lock<HosMutex> lk(this->bus_accessor_mutex);
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

void I2cDriverSession::Start(){
    std::scoped_lock<HosMutex> lk(this->bus_accessor_mutex);
    if (this->open) {
        if (this->bus_accessor->GetOpenSessions() == 1) {
            this->bus_accessor->DoInitialConfig();
        }
    }
}

void I2cDriverSession::Close(){
    std::scoped_lock<HosMutex> lk(this->bus_accessor_mutex);
    if (this->open) {
        this->bus_accessor->Close();
        this->bus_accessor = nullptr;
        this->open = false;
    }
}

bool I2cDriverSession::IsOpen() const{
    return this->open;
}

Result I2cDriverSession::DoTransaction(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, DriverCommand command){
    std::scoped_lock<HosMutex> lk(this->bus_accessor_mutex);
    Result rc;

    if (this->bus_accessor->GetBusy()) {
        return ResultI2cBusBusy;
    }

    this->bus_accessor->OnStartTransaction();

    if (R_SUCCEEDED((rc = this->bus_accessor->StartTransaction(command, this->addressing_mode, this->slave_address)))) {
        switch (command) {
            case DriverCommand_Send:
                rc = this->bus_accessor->Send(reinterpret_cast<const u8 *>(src), num_bytes, option, this->addressing_mode, this->slave_address);
                break;
            case DriverCommand_Receive:
                rc = this->bus_accessor->Receive(reinterpret_cast<u8 *>(dst), num_bytes, option, this->addressing_mode, this->slave_address);
                break;
            default:
                std::abort();
        }
    }

    this->bus_accessor->OnStopTransaction();

    return rc;
}

Result I2cDriverSession::DoTransactionWithRetry(void *dst, const void *src, size_t num_bytes, I2cTransactionOption option, DriverCommand command){
    Result rc;

    size_t i = 0;
    while (true) {
        rc = this->DoTransaction(dst, src, num_bytes, option, command);
        if (rc == ResultI2cTimedOut) {
            i++;
            if (i <= this->max_retries) {
                svcSleepThread(this->retry_wait_time);
                continue;
            }
            return ResultI2cBusBusy;
        } else if (R_FAILED(rc)) {
            return rc;
        }
        return ResultSuccess;
    }
    return rc;
}
