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

#elif defined(ATMOSPHERE_BOARD_QEMU_VIRT)

    #include <mesosphere/board/qemu/virt/kern_cpu_map.hpp>

    namespace ams::kern::cpu {

        using namespace ams::kern::board::qemu::virt::impl::cpu;

    }

#else
    #error "Unknown board for CPU Map"
#endif

namespace ams::kern {

    namespace cpu {

        static constexpr inline size_t NumVirtualCores = BITSIZEOF(u64);

        static constexpr inline u64 VirtualCoreMask    = [] {
            u64 mask = 0;
            for (size_t i = 0; i < NumVirtualCores; ++i) {
                mask |= (UINT64_C(1) << i);
            }
            return mask;
        }();

    }

    static_assert(cpu::NumCores <= cpu::NumVirtualCores);
    static_assert(util::size(cpu::VirtualToPhysicalCoreMap) == cpu::NumVirtualCores);

}
