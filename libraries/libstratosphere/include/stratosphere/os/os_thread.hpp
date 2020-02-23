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
#include "os_common_types.hpp"
#include "os_memory_common.hpp"

namespace ams::os {

    class Thread {
        NON_COPYABLE(Thread);
        NON_MOVEABLE(Thread);
        private:
            ::Thread thr;
        public:
            constexpr Thread() : thr{} { /* ... */ }

            Result Initialize(ThreadFunc entry, void *arg, void *stack_mem, size_t stack_sz, int prio, int cpuid = -2) {
                return threadCreate(&this->thr, entry, arg, stack_mem, stack_sz, prio, cpuid);
            }

            Result Initialize(ThreadFunc entry, void *arg, size_t stack_sz, int prio, int cpuid = -2) {
                return threadCreate(&this->thr, entry, arg, nullptr, stack_sz, prio, cpuid);
            }

            Handle GetHandle() const {
                return this->thr.handle;
            }

            Result Start() {
                return threadStart(&this->thr);
            }

            Result Wait() {
                return threadWaitForExit(&this->thr);
            }

            Result Join() {
                R_TRY(threadWaitForExit(&this->thr));
                R_TRY(threadClose(&this->thr));
                return ResultSuccess();
            }

            Result CancelSynchronization() {
                return svcCancelSynchronization(this->thr.handle);
            }
    };

    template<size_t StackSize>
    class StaticThread {
        NON_COPYABLE(StaticThread);
        NON_MOVEABLE(StaticThread);
        static_assert(util::IsAligned(StackSize, os::MemoryPageSize), "StaticThread must have aligned resource size");
        private:
            alignas(os::MemoryPageSize) u8 stack_mem[StackSize];
            ::Thread thr;
        public:
            constexpr StaticThread() : stack_mem{}, thr{} { /* ... */ }

            constexpr StaticThread(ThreadFunc entry, void *arg, int prio, int cpuid = -2) : StaticThread() {
                R_ABORT_UNLESS(this->Initialize(entry, arg, prio, cpuid));
            }

            Result Initialize(ThreadFunc entry, void *arg, int prio, int cpuid = -2) {
                return threadCreate(&this->thr, entry, arg, this->stack_mem, StackSize, prio, cpuid);
            }

            Handle GetHandle() const {
                return this->thr.handle;
            }

            Result Start() {
                return threadStart(&this->thr);
            }

            Result Wait() {
                return threadWaitForExit(&this->thr);
            }

            Result Join() {
                R_TRY(threadWaitForExit(&this->thr));
                R_TRY(threadClose(&this->thr));
                return ResultSuccess();
            }

            Result CancelSynchronization() {
                return svcCancelSynchronization(this->thr.handle);
            }
    };

    NX_INLINE u32 GetCurrentThreadPriority() {
        u32 prio;
        R_ABORT_UNLESS(svcGetThreadPriority(&prio, CUR_THREAD_HANDLE));
        return prio;
    }

}
