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
#include <vapours.hpp>
#include <stratosphere/i2c/i2c_types.hpp>
#include <stratosphere/ddsf.hpp>

namespace ams::i2c::driver {

    class I2cDeviceProperty;

    class II2cDriver : public ::ams::ddsf::IDriver {
        NON_COPYABLE(II2cDriver);
        NON_MOVEABLE(II2cDriver);
        AMS_DDSF_CASTABLE_TRAITS(ams::i2c::driver::II2cDriver, ::ams::ddsf::IDriver);
        public:
            II2cDriver() : IDriver() { /* ... */ }
            virtual ~II2cDriver() { /* ... */ }

            virtual void InitializeDriver() = 0;
            virtual void FinalizeDriver()   = 0;

            virtual Result InitializeDevice(I2cDeviceProperty *device) = 0;
            virtual void FinalizeDevice(I2cDeviceProperty *device)     = 0;

            virtual Result Send(I2cDeviceProperty *device, const void *src, size_t src_size, TransactionOption option) = 0;
            virtual Result Receive(void *dst, size_t dst_size, I2cDeviceProperty *device, TransactionOption option)    = 0;

            virtual os::SdkMutex &GetTransactionOrderMutex() = 0;

            virtual void SuspendBus()      = 0;
            virtual void SuspendPowerBus() = 0;

            virtual void ResumeBus()       = 0;
            virtual void ResumePowerBus()  = 0;

            virtual const DeviceCode &GetDeviceCode() const = 0;
    };

}
