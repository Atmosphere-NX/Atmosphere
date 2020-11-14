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
#include "boot_driver_management.hpp"

namespace ams::boot {

    void InitializeGpioDriverLibrary() {
        /* Initialize the gpio client library with the server manager object. */
        gpio::InitializeWith(gpio::server::GetServiceObject());

        /* Initialize the board driver without enabling interrupt handlers. */
        gpio::driver::board::Initialize(false);

        /* Initialize the driver library. */
        gpio::driver::Initialize();
    }

    void InitializeI2cDriverLibrary() {
        /* Initialize the i2c client library with the server manager object. */
        i2c::InitializeWith(i2c::server::GetServiceObject(), i2c::server::GetServiceObjectPowerBus());

        /* Initialize the board driver. */
        i2c::driver::board::Initialize();

        /* Initialize the driver library. */
        i2c::driver::Initialize();

        /* Initialize the pwm client library with the server manager object. */
        pwm::InitializeWith(pwm::server::GetServiceObject());

        /* Initialize the pwm board driver. */
        pwm::driver::board::Initialize();

        /* Initialize the pwm driver library. */
        pwm::driver::Initialize();
    }

    void FinalizeI2cDriverLibrary() {
        /* Finalize the i2c driver library. */
        i2c::driver::Finalize();

        /* Finalize the i2c client library. */
        i2c::Finalize();

        /* NOTE: Unknown finalize function is called here by Nintendo. */

        /* Finalize the pwm client library. */
        pwm::Finalize();
    }

}
