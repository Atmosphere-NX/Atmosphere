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
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    class DebugHorizonImpl {
        public:
            static uintptr_t GetCurrentStackPointer() {
                uintptr_t v;
                __asm__ __volatile__("mov %[v], sp" : [v]"=&r"(v) :: "memory");
                return v;
            }

            static void GetCurrentStackInfo(uintptr_t *out_stack, size_t *out_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(out_stack != nullptr);
                AMS_ASSERT(out_size != nullptr);

                /* Get the current thread. */
                auto *cur_thread = os::impl::GetCurrentThread();
                auto *cur_fiber  = cur_thread->current_fiber;

                /* Get the current stack pointer. */
                uintptr_t cur_sp = GetCurrentStackPointer();

                /* Determine current stack extents, TODO Fiber */
                uintptr_t stack_top = reinterpret_cast<uintptr_t>(cur_fiber == nullptr ? cur_thread->stack : /* TODO: cur_fiber->stack */ nullptr);
                size_t stack_size   = reinterpret_cast<size_t>(cur_fiber == nullptr ? cur_thread->stack_size : /* TODO: cur_fiber->stack_size */ 0);

                uintptr_t stack_bottom = stack_top + stack_size;

                /* TODO: User exception handler, check if stack is out of range and use exception stack. */

                /* Check that the stack pointer is in bounds. */
                AMS_ABORT_UNLESS((stack_top <= cur_sp) && (cur_sp < stack_bottom));

                /* Set the output. */
                *out_stack = stack_top;
                *out_size  = stack_size;
            }

            static void QueryMemoryInfo(os::MemoryInfo *out) {
                AMS_UNUSED(out);
                AMS_ABORT("TODO: Horizon QueryMemoryInfo");
            }

            static Tick GetIdleTickCount() {
                u64 value;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), svc::InfoType_IdleTickCount, svc::InvalidHandle, static_cast<u64>(-1)));

                return os::Tick(value);
            }

            static Tick GetThreadTickCount() {
                u64 value;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), svc::InfoType_ThreadTickCount, svc::PseudoHandle::CurrentThread, static_cast<u64>(-1)));

                return os::Tick(value);
            }

            static int GetFreeThreadCount() {
                u64 value;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), svc::InfoType_FreeThreadCount, svc::PseudoHandle::CurrentProcess, 0));

                AMS_ASSERT(value <= static_cast<u64>(std::numeric_limits<int>::max()));

                return static_cast<int>(value);
            }
    };

    using DebugImpl = DebugHorizonImpl;

}