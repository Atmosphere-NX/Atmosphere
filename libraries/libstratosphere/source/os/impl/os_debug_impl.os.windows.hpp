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
#include <stratosphere/windows.hpp>

namespace ams::os::impl {

    class DebugWindowsImpl {
        public:
            static void GetCurrentStackInfo(uintptr_t *out_stack, size_t *out_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(out_stack != nullptr);
                AMS_ASSERT(out_size != nullptr);

                /* Get the current stack by NT_TIB */
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Warray-bounds"
                auto *tib = reinterpret_cast<NT_TIB *>(::NtCurrentTeb());
                #pragma GCC diagnostic pop

                *out_stack = reinterpret_cast<uintptr_t>(tib->StackLimit);
                *out_size  = reinterpret_cast<uintptr_t>(tib->StackBase) - reinterpret_cast<uintptr_t>(tib->StackLimit);
            }

            static void QueryMemoryInfo(os::MemoryInfo *out) {
                AMS_UNUSED(out);
                AMS_ABORT("TODO: Windows QueryMemoryInfo");
            }

            static Tick GetIdleTickCount() {
                return os::Tick(0);
            }

            static Tick GetThreadTickCount() {
                return os::Tick(0);
            }

            static int GetFreeThreadCount() {
                return 0;
            }
    };

    using DebugImpl = DebugWindowsImpl;

}