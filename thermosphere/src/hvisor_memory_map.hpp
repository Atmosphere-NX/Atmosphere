/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "defines.hpp"

namespace ams::hvisor {

    class MemoryMap final {
        NON_COPYABLE(MemoryMap);
        NON_MOVEABLE(MemoryMap);
        private:
            // Maps to AttrIndx[2:0]
            enum MemType {
                Memtype_Device_nGnRnE           = 0,
                Memtype_Normal                  = 1,
                Memtype_Device_nGnRE            = 2,
                Memtype_Normal_Uncacheable      = 3,
                Memtype_Guest_Slot              = 4,
            };

            struct LoadImageLayout {
                uintptr_t startPa;
                size_t imageSize; // "image" includes "real" BSS but not tempbss

                uintptr_t tempPa;
                size_t maxTempSize;
                size_t tempSize;

                uintptr_t vbar;
            };
            static_assert(std::is_standard_layout_v<LoadImageLayout>);
            static_assert(std::is_trivial_v<LoadImageLayout>);
        private:
            static LoadImageLayout imageLayout;
            static uintptr_t currentPlatformMmioPage;

        public:
            static constexpr u32 addressSpaceSize = 39;

            // The following come from the fact we're using a recursive page table:
            static constexpr uintptr_t selfL2VaRange        = 0x7FC0000000ul;   // = 511 << 31
            static constexpr uintptr_t selfL3VaRange        = 0x7FFFE00000ul;   // = 511 << 31 | 511 << 21
            static constexpr uintptr_t ttblVa               = 0x7FFFFFF000ul;   // = 511 << 31 | 511 << 21 | 511 << 12
            static constexpr uintptr_t maxVa                = 0x7FFFFFFFFFul;   // = all 39 bits set

            static constexpr size_t crashStacksSize         = 0x1000ul;

            // Do not use the first 0x10000 to allow for L1/L2 mappings...
            static constexpr uintptr_t imageVa              = selfL3VaRange + 0x10000;
            static constexpr uintptr_t crashStacksBottomVa  = selfL3VaRange + 0x40000;
            static constexpr uintptr_t crashStacksTopVa     = crashStacksBottomVa + crashStacksSize;
            static constexpr uintptr_t guestMemVa           = selfL3VaRange + 0x50000;
            static constexpr uintptr_t stacksBottomVa       = selfL3VaRange + 0x60000;

            static constexpr uintptr_t mmioBaseVa           = selfL3VaRange + 0x80000;
            static constexpr uintptr_t gicdVa               = mmioBaseVa + 0x0000;
            static constexpr uintptr_t giccVa               = mmioBaseVa + 0x1000;
            static constexpr uintptr_t gichVa               = mmioBaseVa + 0x3000;

            static constexpr uintptr_t mmioPlatBaseVa       = selfL3VaRange + 0x90000;

            static uintptr_t GetStartPa() { return imageLayout.startPa; }

            // Called before MMU is enabled. EnableMmu must not use a stack frame
            static void SetupMmu(const LoadImageLayout *layout);
            static std::array<uintptr_t, 2> EnableMmuGetStacks(const LoadImageLayout *layout, u32 coreId);

            // Caller is expected to invalidate TLB + barrier at some point
            static uintptr_t MapPlatformMmio(uintptr_t pa, size_t size);

            // Caller is expected to disable interrupts, etc, etc.
            static uintptr_t MapGuestPage(uintptr_t pa, u64 memAttribs, u64 shareability);
            static uintptr_t UnmapGuestPage();

        public:
            constexpr MemoryMap() = delete;
    };

}
