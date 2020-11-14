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

#include "impl/pwm_driver_core.hpp"

namespace ams::pwm::driver {

    void RegisterDriver(IPwmDriver *driver) {
        return impl::RegisterDriver(driver);
    }

    void UnregisterDriver(IPwmDriver *driver) {
        return impl::UnregisterDriver(driver);
    }

    Result RegisterDeviceCode(DeviceCode device_code, IPwmDevice *device) {
        return impl::RegisterDeviceCode(device_code, device);
    }

    bool UnregisterDeviceCode(DeviceCode device_code) {
        return impl::UnregisterDeviceCode(device_code);
    }

}
