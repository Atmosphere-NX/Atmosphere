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

namespace ams::boot {

    constexpr inline dd::PhysicalAddress PmcBase = 0x7000E400;

    /* PMC Access Utilities. */
    u32 ReadPmcRegister(dd::PhysicalAddress phys_addr);
    void WritePmcRegister(dd::PhysicalAddress phys_addr, u32 value, u32 mask = std::numeric_limits<u32>::max());

}
