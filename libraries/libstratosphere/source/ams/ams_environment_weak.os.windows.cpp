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

extern "C" char **__real___p__acmdln(void);
extern "C" _invalid_parameter_handler __real__set_invalid_parameter_handler(_invalid_parameter_handler);

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

    /* On windows, mingw may attempt to call malloc before we've initialized globals to set up the command line. */
    /* We perform some critical init here, to make that work. */
    char **__wrap___p__acmdln(void) {
        ::ams::os::Initialize();
        return __real___p__acmdln();
    }

    /* On some mingw gcc versions, acmdln isn't used, so we need to hook a different part of crt init. */
    _invalid_parameter_handler __wrap__set_invalid_parameter_handler(_invalid_parameter_handler handler) {
        ::ams::os::Initialize();
        return __real__set_invalid_parameter_handler(handler);

    }

}
