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

    class DebugLinuxImpl {
        public:
            static void GetCurrentStackInfo(uintptr_t *out_stack, size_t *out_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(out_stack != nullptr);
                AMS_ASSERT(out_size != nullptr);

                /* Get the current stack by pthread */
                pthread_attr_t attr;
                pthread_attr_init(std::addressof(attr));
                ON_SCOPE_EXIT { pthread_attr_destroy(std::addressof(attr)); };

                const auto getattr_res = pthread_getattr_np(pthread_self(), std::addressof(attr));
                AMS_ABORT_UNLESS(getattr_res == 0);

                /* Get the thread satck. */
                void *base = nullptr;
                size_t size  = 0;
                const auto getstack_res = pthread_attr_getstack(std::addressof(attr), std::addressof(base), std::addressof(size));
                AMS_ABORT_UNLESS(getstack_res == 0);

                *out_stack = reinterpret_cast<uintptr_t>(base);
                *out_size  = size;
            }

            static void QueryMemoryInfo(os::MemoryInfo *out) {
                AMS_UNUSED(out);
                AMS_ABORT("TODO: Linux QueryMemoryInfo");
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

    using DebugImpl = DebugLinuxImpl;

}