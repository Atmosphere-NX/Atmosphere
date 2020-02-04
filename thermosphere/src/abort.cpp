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

#include "defines.hpp"

extern "C" {

    /* Redefine abort to trigger these handlers. */
    void abort();

    /* Redefine C++ exception handlers. Requires wrap linker flag. */
    #define WRAP_ABORT_FUNC(func) void NORETURN __wrap_##func(void) { abort(); __builtin_unreachable(); }
    WRAP_ABORT_FUNC(__cxa_pure_virtual)
    WRAP_ABORT_FUNC(__cxa_throw)
    WRAP_ABORT_FUNC(__cxa_rethrow)
    WRAP_ABORT_FUNC(__cxa_allocate_exception)
    WRAP_ABORT_FUNC(__cxa_free_exception)
    WRAP_ABORT_FUNC(__cxa_begin_catch)
    WRAP_ABORT_FUNC(__cxa_end_catch)
    WRAP_ABORT_FUNC(__cxa_call_unexpected)
    WRAP_ABORT_FUNC(__cxa_call_terminate)
    WRAP_ABORT_FUNC(__gxx_personality_v0)
    WRAP_ABORT_FUNC(_ZSt19__throw_logic_errorPKc)
    WRAP_ABORT_FUNC(_ZSt20__throw_length_errorPKc)
    WRAP_ABORT_FUNC(_ZNSt11logic_errorC2EPKc)

    /* TODO: We may wish to consider intentionally not defining an _Unwind_Resume wrapper. */
    /* This would mean that a failure to wrap all exception functions is a linker error. */
    WRAP_ABORT_FUNC(_Unwind_Resume)
    #undef WRAP_ABORT_FUNC

}

/* Custom abort handler, so that std::abort will trigger these. */
void abort()
{
#ifndef PLATFORM_QEMU
    __builtin_trap();
#endif
    for (;;);
}