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
#include <stratosphere.hpp>
#include "os_thread_manager_types.hpp"
#include "os_resource_manager.hpp"

namespace ams::os::impl {

    constexpr inline s32 CoreAffinityMaskBitWidth = BITSIZEOF(u64);

    ALWAYS_INLINE ThreadManager &GetThreadManager() {
        return GetResourceManager().GetThreadManager();
    }

    ALWAYS_INLINE ThreadType *GetCurrentThread() {
        return GetThreadManager().GetCurrentThread();
    }

    #if !defined(AMS_OS_IMPL_USE_PTHREADS)
    ALWAYS_INLINE NativeHandle GetCurrentThreadHandle() {
        #if defined(ATMOSPHERE_OS_HORIZON)
        return ::threadGetCurHandle();
        #else
        return GetCurrentThread()->native_handle;
        #endif
    }
    #endif

    void SetupThreadObjectUnsafe(ThreadType *thread, void *platform, ThreadFunction function, void *arg, void *stack, size_t stack_size, s32 priority);

}
