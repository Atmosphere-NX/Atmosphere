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

    namespace os {

        void Initialize();

    }

    void *Malloc(size_t size) {
        return std::malloc(size);
    }

    void Free(void *ptr) {
        return std::free(ptr);
    }

    void *MallocForRapidJson(size_t size) {
        return std::malloc(size);
    }

    void *ReallocForRapidJson(void *ptr, size_t size) {
        return std::realloc(ptr, size);
    }

    void FreeForRapidJson(void *ptr) {
        return std::free(ptr);
    }

    NORETURN void AbortImpl() {
        std::abort();
    }

}

extern "C" {

    /* Redefine C++ exception handlers. Requires wrap linker flag. */
    #define WRAP_ABORT_FUNC(func) void NORETURN __wrap_##func(void) { std::abort(); __builtin_unreachable(); }
    WRAP_ABORT_FUNC(__cxa_pure_virtual)
    #undef WRAP_ABORT_FUNC

}
