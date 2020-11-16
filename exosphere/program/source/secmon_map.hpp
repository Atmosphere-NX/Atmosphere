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
#include <exosphere.hpp>

namespace ams::secmon {

    bool IsPhysicalMemoryAddress(uintptr_t address);
    size_t GetPhysicalMemorySize();

    void UnmapTzram();

    uintptr_t MapSmcUserPage(uintptr_t address);
    void UnmapSmcUserPage();

    uintptr_t MapAtmosphereIramPage(uintptr_t address);
    void UnmapAtmosphereIramPage();

    uintptr_t MapAtmosphereUserPage(uintptr_t address);
    void UnmapAtmosphereUserPage();

    void MapDramForMarikoProgram();

}