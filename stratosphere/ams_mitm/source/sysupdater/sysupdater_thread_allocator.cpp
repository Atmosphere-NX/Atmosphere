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
#include "sysupdater_thread_allocator.hpp"

namespace ams::mitm::sysupdater {

    Result ThreadAllocator::Allocate(ThreadInfo *out) {
        std::scoped_lock lk(this->mutex);

        for (int i = 0; i < this->thread_count; ++i) {
            const u64 mask = (static_cast<u64>(1) << i);
            if ((this->bitmap & mask) == 0) {
                *out = {
                    .thread     = this->thread_list + i,
                    .priority   = this->thread_priority,
                    .stack      = this->stack_heap + (this->stack_size * i),
                    .stack_size = this->stack_size,
                };
                this->bitmap |= mask;
                return ResultSuccess();
            }
        }

        return ns::ResultOutOfMaxRunningTask();
    }

    void ThreadAllocator::Free(const ThreadInfo &info) {
        std::scoped_lock lk(this->mutex);

        for (int i = 0; i < this->thread_count; ++i) {
            if (info.thread == std::addressof(this->thread_list[i])) {
                const u64 mask = (static_cast<u64>(1) << i);
                this->bitmap &= ~mask;
                return;
            }
        }

        AMS_ABORT("Invalid thread passed to ThreadAllocator::Free");
    }

}
