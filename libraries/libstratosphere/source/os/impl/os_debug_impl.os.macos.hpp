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

namespace ams::os::impl {

    class DebugMacosImpl {
        public:
            static void GetCurrentStackInfo(uintptr_t *out_stack, size_t *out_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(out_stack != nullptr);
                AMS_ASSERT(out_size != nullptr);

                /* Get the current pthread. */
                const auto self = pthread_self();

                /* Get the thread stack. */
                uintptr_t stack_bottom = reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(self));
                size_t stack_size      = pthread_get_stacksize_np(self);

                uintptr_t stack_top    = stack_bottom - stack_size;

                *out_stack = stack_top;
                *out_size  = stack_size;
            }

            static void QueryMemoryInfo(os::MemoryInfo *out) {
                AMS_UNUSED(out);
                AMS_ABORT("TODO: macOS QueryMemoryInfo");
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

    using DebugImpl = DebugMacosImpl;

}