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

namespace ams::os::impl {

    #if defined(ATMOSPHERE_ARCH_ARM64)

        ALWAYS_INLINE void FenceMemoryStoreStore() { __asm__ __volatile__("dmb ishst" ::: "memory"); }
        ALWAYS_INLINE void FenceMemoryStoreLoad()  { __asm__ __volatile__("dmb ish"   ::: "memory"); }
        ALWAYS_INLINE void FenceMemoryStoreAny()   { __asm__ __volatile__("dmb ish"   ::: "memory"); }

        ALWAYS_INLINE void FenceMemoryLoadStore()  { __asm__ __volatile__("dmb ishld" ::: "memory"); }
        ALWAYS_INLINE void FenceMemoryLoadLoad()   { __asm__ __volatile__("dmb ishld" ::: "memory"); }
        ALWAYS_INLINE void FenceMemoryLoadAny()    { __asm__ __volatile__("dmb ishld" ::: "memory"); }

        ALWAYS_INLINE void FenceMemoryAnyStore()   { __asm__ __volatile__("dmb ish"   ::: "memory"); }
        ALWAYS_INLINE void FenceMemoryAnyLoad()    { __asm__ __volatile__("dmb ish"   ::: "memory"); }
        ALWAYS_INLINE void FenceMemoryAnyAny()     { __asm__ __volatile__("dmb ish"   ::: "memory"); }

    #else

        #error "Unknown architecture for os::impl::FenceMemory* (Horizon)"

    #endif

}
