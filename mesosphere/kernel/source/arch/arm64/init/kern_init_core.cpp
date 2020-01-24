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
#include <mesosphere.hpp>

namespace ams::kern::init {

    namespace {

        /* Global Allocator. */
        KInitialPageAllocator g_initial_page_allocator;

        /* Global initial arguments array. */
        KInitArguments g_init_arguments[cpu::NumCores];

    }

    void InitializeCore(uintptr_t arg0, uintptr_t initial_page_allocator_state) {
        /* TODO */
    }

    KPhysicalAddress GetInitArgumentsAddress(s32 core) {
        return KPhysicalAddress(std::addressof(g_init_arguments[core]));
    }

    void InitializeDebugRegisters() {
        /* TODO */
    }

    void InitializeExceptionVectors() {
        /* TODO */
    }

}