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
#include <vapours/svc/svc_common.hpp>

#if defined(ATMOSPHERE_ARCH_ARM64)

    #include <vapours/svc/arch/arm64/svc_thread_local_region.hpp>
    namespace ams::svc {
        using ams::svc::arch::arm64::ThreadLocalRegion;
        using ams::svc::arch::arm64::GetThreadLocalRegion;
    }

#elif defined(ATMOSPHERE_ARCH_ARM)

    #include <vapours/svc/arch/arm/svc_thread_local_region.hpp>
    namespace ams::svc {
        using ams::svc::arch::arm::ThreadLocalRegion;
        using ams::svc::arch::arm::GetThreadLocalRegion;
    }

#else

    #error "Unknown architecture for svc::ThreadLocalRegion"

#endif

namespace ams::svc {

    constexpr inline size_t ThreadLocalRegionSize = 0x200;
    static_assert(sizeof(::ams::svc::ThreadLocalRegion) == ThreadLocalRegionSize);

}