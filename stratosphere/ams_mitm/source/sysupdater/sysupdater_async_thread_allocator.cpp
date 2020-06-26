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
#include <stratosphere.hpp>
#include "sysupdater_async_thread_allocator.hpp"

namespace ams::mitm::sysupdater {

    namespace {

        constexpr inline int AsyncThreadCount = 1;
        constexpr inline size_t AsyncThreadStackSize = 16_KB;

        os::ThreadType g_async_threads[AsyncThreadCount];
        alignas(os::ThreadStackAlignment) u8 g_async_thread_stack_heap[AsyncThreadCount * AsyncThreadStackSize];

        constinit ThreadAllocator g_async_thread_allocator(g_async_threads, AsyncThreadCount, os::InvalidThreadPriority, g_async_thread_stack_heap, sizeof(g_async_thread_stack_heap), AsyncThreadStackSize);

    }

    ThreadAllocator *GetAsyncThreadAllocator() {
        return std::addressof(g_async_thread_allocator);
    }


}
