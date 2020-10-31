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

#include "impl/gpio_driver_core.hpp"

namespace ams::gpio::driver {

    void RegisterDriver(IGpioDriver *driver) {
        return impl::RegisterDriver(driver);
    }

    void UnregisterDriver(IGpioDriver *driver) {
        return impl::UnregisterDriver(driver);
    }

    Result RegisterDeviceCode(DeviceCode device_code, Pad *pad) {
        return impl::RegisterDeviceCode(device_code, pad);
    }

    bool UnregisterDeviceCode(DeviceCode device_code) {
        return impl::UnregisterDeviceCode(device_code);
    }

    void RegisterInterruptHandler(ddsf::IEventHandler *handler) {
        return impl::RegisterInterruptHandler(handler);
    }

    void UnregisterInterruptHandler(ddsf::IEventHandler *handler) {
        return impl::UnregisterInterruptHandler(handler);
    }

    void SetInitialGpioConfig() {
        return board::SetInitialGpioConfig();
    }

    void SetInitialWakePinConfig() {
        return board::SetInitialWakePinConfig();
    }

}
