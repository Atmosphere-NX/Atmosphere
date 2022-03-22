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
#include "powctl_i_power_control_driver.hpp"

namespace ams::powctl::impl {

    void InitializeDrivers();
    void FinalizeDrivers();

    void RegisterDriver(IPowerControlDriver *driver);
    void UnregisterDriver(IPowerControlDriver *driver);

    Result RegisterDeviceCode(DeviceCode device_code, IDevice *device);
    bool UnregisterDeviceCode(DeviceCode device_code);

    void RegisterInterruptHandler(ddsf::IEventHandler *handler);
    void UnregisterInterruptHandler(ddsf::IEventHandler *handler);

    Result FindDevice(IDevice **out, DeviceCode device_code);

}
