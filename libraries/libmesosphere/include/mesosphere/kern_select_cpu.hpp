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
#include <mesosphere/kern_common.hpp>

#ifdef ATMOSPHERE_ARCH_ARM64
    #include <mesosphere/arch/arm64/kern_cpu.hpp>

    namespace ams::kern::cpu {

        using namespace ams::kern::arch::arm64::cpu;

    }

#else
    #error "Unknown architecture for CPU"
#endif

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX

    #include <mesosphere/board/nintendo/nx/kern_cpu_map.hpp>

    namespace ams::kern::cpu {

        using namespace ams::kern::board::nintendo::nx::impl::cpu;

    }

#else
    #error "Unknown board for CPU Map"
#endif

namespace ams::kern {

    static_assert(cpu::NumCores <= static_cast<s32>(BITSIZEOF(u64)));
    static_assert(util::size(cpu::VirtualToPhysicalCoreMap) == BITSIZEOF(u64));

}
