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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include <stratosphere/os/impl/os_memory_fence_api.os.horizon.hpp>
#else
    #error "Unknown os for os::MemoryFence*"
#endif

namespace ams::os {

    ALWAYS_INLINE void FenceMemoryStoreStore() { return impl::FenceMemoryStoreStore(); }
    ALWAYS_INLINE void FenceMemoryStoreLoad()  { return impl::FenceMemoryStoreLoad(); }
    ALWAYS_INLINE void FenceMemoryStoreAny()   { return impl::FenceMemoryStoreAny(); }

    ALWAYS_INLINE void FenceMemoryLoadStore() { return impl::FenceMemoryLoadStore(); }
    ALWAYS_INLINE void FenceMemoryLoadLoad()  { return impl::FenceMemoryLoadLoad(); }
    ALWAYS_INLINE void FenceMemoryLoadAny()   { return impl::FenceMemoryLoadAny(); }

    ALWAYS_INLINE void FenceMemoryAnyStore() { return impl::FenceMemoryLoadStore(); }
    ALWAYS_INLINE void FenceMemoryAnyLoad()  { return impl::FenceMemoryLoadLoad(); }
    ALWAYS_INLINE void FenceMemoryAnyAny()   { return impl::FenceMemoryLoadAny(); }

}
