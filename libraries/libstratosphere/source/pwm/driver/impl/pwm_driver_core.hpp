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

namespace ams::pwm::driver::impl {

    void InitializeDrivers();
    void FinalizeDrivers();

    void RegisterDriver(IPwmDriver *driver);
    void UnregisterDriver(IPwmDriver *driver);

    Result RegisterDeviceCode(DeviceCode device_code, IPwmDevice *device);
    bool UnregisterDeviceCode(DeviceCode device_code);

    Result FindDevice(IPwmDevice **out, DeviceCode device_code);
    Result FindDeviceByChannelIndex(IPwmDevice **out, int channel);

}
