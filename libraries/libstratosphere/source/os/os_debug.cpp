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
#include <stratosphere.hpp>
#include "impl/os_debug_impl.hpp"

namespace ams::os {

    void GetCurrentStackInfo(uintptr_t *out_stack, size_t *out_size) {
        /* Get the current stack info. */
        uintptr_t stack_top = 0;
        size_t stack_size   = 0;
        impl::DebugImpl::GetCurrentStackInfo(std::addressof(stack_top), std::addressof(stack_size));

        /* Basic sanity check. */
        uintptr_t sp = reinterpret_cast<uintptr_t>(std::addressof(stack_top));
        AMS_ASSERT((stack_top <= sp) && (sp < (stack_top + stack_size)));
        AMS_UNUSED(sp);

        /* Set the output. */
        if (out_stack != nullptr) {
            *out_stack = stack_top;
        }
        if (out_size != nullptr) {
            *out_size = stack_size;
        }
    }

    void QueryMemoryInfo(MemoryInfo *out) {
        return impl::DebugImpl::QueryMemoryInfo(out);
    }

    Tick GetIdleTickCount() {
        return impl::DebugImpl::GetIdleTickCount();
    }

    int GetFreeThreadCount() {
        return impl::DebugImpl::GetFreeThreadCount();
    }

}
