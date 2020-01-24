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
#include <stratosphere.hpp>

/* This forward declares the functionality from pcv that i2c::driver uses. */
/* This allows for overriding at compile-time (e.g., for boot sysmodule). */
namespace ams::pcv {

    void Initialize();
    void Finalize();
    Result SetClockRate(PcvModule module, u32 hz);
    Result SetClockEnabled(PcvModule module, bool enabled);
    Result SetVoltageEnabled(u32 domain, bool enabled);
    Result SetVoltageValue(u32 domain, u32 voltage);
    Result SetReset(PcvModule module, bool reset);

}
