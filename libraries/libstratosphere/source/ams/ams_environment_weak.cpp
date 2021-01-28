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

namespace ams {

    WEAK_SYMBOL void *Malloc(size_t size) {
        return std::malloc(size);
    }

    WEAK_SYMBOL void Free(void *ptr) {
        return std::free(ptr);
    }

}

extern "C" {

    /* Redefine C++ exception handlers. Requires wrap linker flag. */
    #define WRAP_ABORT_FUNC(func) void NORETURN __wrap_##func(void) { abort(); __builtin_unreachable(); }
    WRAP_ABORT_FUNC(__cxa_pure_virtual)
    #undef WRAP_ABORT_FUNC

}
