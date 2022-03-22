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
#include <vapours.hpp>
#include <stratosphere/gpio/gpio_types.hpp>
#include <stratosphere/gpio/driver/gpio_i_gpio_driver.hpp>

namespace ams::gpio::driver {

    void RegisterDriver(IGpioDriver *driver);
    void UnregisterDriver(IGpioDriver *driver);

    Result RegisterDeviceCode(DeviceCode device_code, Pad *pad);
    bool UnregisterDeviceCode(DeviceCode device_code);

    void RegisterInterruptHandler(ddsf::IEventHandler *handler);
    void UnregisterInterruptHandler(ddsf::IEventHandler *handler);

    void SetInitialGpioConfig();
    void SetInitialWakePinConfig();

}
