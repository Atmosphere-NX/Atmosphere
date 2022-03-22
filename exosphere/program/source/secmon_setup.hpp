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
#include <exosphere.hpp>

namespace ams::secmon {

    constexpr inline u64 MemoryAttributeIndexNormal = 0;
    constexpr inline u64 MemoryAttributeIndexDevice = 1;

    constexpr inline int KernelCarveoutCount = 2;

    constexpr size_t CarveoutSizeMax = 512_MB - 128_KB;

    void SetupCpuMemoryControllersEnableMmu();
    void SetupCpuCoreContext();
    void SetupCpuSErrorDebug();

    void SetupSocDmaControllers();
    void SetupSocSecurity();
    void SetupSocProtections();

    void SetupPmcAndMcSecure();

    void Setup1();

    void SaveSecurityEngineAesKeySlotTestVector();

    void SetKernelCarveoutRegion(int index, uintptr_t address, size_t size);

}