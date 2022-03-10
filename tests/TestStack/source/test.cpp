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

namespace ams {

    void Main() {
        printf("Getting thread stack\n");
        {
            uintptr_t stack = 0;
            size_t stack_size = 0;
            os::GetCurrentStackInfo(std::addressof(stack), std::addressof(stack_size));

            printf("Got thread stack: %p-%p\n", reinterpret_cast<void *>(stack), reinterpret_cast<void *>(stack + stack_size));

            const uintptr_t stack_var_addr = reinterpret_cast<uintptr_t>(std::addressof(stack));
            printf("&stack variable address: %p\n", reinterpret_cast<void *>(stack_var_addr));
            AMS_ASSERT(stack <= stack_var_addr && stack_var_addr < stack + stack_size);
        }
        printf("All tests completed!\n");
    }

}