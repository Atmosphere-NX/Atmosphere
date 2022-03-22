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

namespace ams::mitm::sysupdater {

    struct ThreadInfo {
        os::ThreadType *thread;
        int priority;
        void *stack;
        size_t stack_size;
    };

    /* NOTE: Nintendo uses a util::BitArray, but this seems excessive. */
    class ThreadAllocator {
        private:
            os::ThreadType *m_thread_list;
            const int m_thread_priority;
            const int m_thread_count;
            u8 *m_stack_heap;
            const size_t m_stack_heap_size;
            const size_t m_stack_size;
            u64 m_bitmap;
            os::SdkMutex m_mutex;
        public:
            constexpr ThreadAllocator(os::ThreadType *thread_list, int count, int priority, u8 *stack_heap, size_t stack_heap_size, size_t stack_size)
                : m_thread_list(thread_list), m_thread_priority(priority), m_thread_count(count), m_stack_heap(stack_heap), m_stack_heap_size(stack_heap_size), m_stack_size(stack_size), m_bitmap()
            {
                AMS_ASSERT(count <= static_cast<int>(stack_heap_size / stack_size));
                AMS_ASSERT(count <= static_cast<int>(BITSIZEOF(m_bitmap)));
            }

            Result Allocate(ThreadInfo *out);
            void Free(const ThreadInfo &info);
    };

}
