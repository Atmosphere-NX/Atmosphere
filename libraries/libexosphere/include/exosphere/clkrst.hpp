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

namespace ams::clkrst {

    void SetRegisterAddress(uintptr_t address);

    void SetFuseVisibility(bool visible);

    void EnableUartAClock();
    void EnableUartBClock();
    void EnableUartCClock();
    void EnableActmonClock();
    void EnableI2c1Clock();
    void EnableI2c5Clock();

    void DisableI2c1Clock();

}