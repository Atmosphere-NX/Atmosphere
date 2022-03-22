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
#include <exosphere/fuse.hpp>

namespace ams::pinmux {

    void SetRegisterAddress(uintptr_t pinmux_address, uintptr_t gpio_address);

    void SetupFirst(fuse::HardwareType hw_type);

    void SetupUartA();
    void SetupUartB();
    void SetupUartC();
    void SetupI2c1();
    void SetupI2c5();

    void SetupVolumeButton();
    void SetupHomeButton();

}
