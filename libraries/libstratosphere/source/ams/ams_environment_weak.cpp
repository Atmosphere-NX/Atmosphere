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

extern "C" void NORETURN __real_exit(int rc);

namespace ams {

    WEAK_SYMBOL void *Malloc(size_t size) {
        return std::malloc(size);
    }

    WEAK_SYMBOL void Free(void *ptr) {
        return std::free(ptr);
    }

    WEAK_SYMBOL void *MallocForRapidJson(size_t size) {
        return std::malloc(size);
    }

    WEAK_SYMBOL void *ReallocForRapidJson(void *ptr, size_t size) {
        return std::realloc(ptr, size);
    }

    WEAK_SYMBOL void FreeForRapidJson(void *ptr) {
        return std::free(ptr);
    }

    WEAK_SYMBOL void NORETURN Exit(int rc) {
        __real_exit(rc);
        __builtin_unreachable();
    }

    NOINLINE NORETURN void AbortImpl() {
        /* Just perform a data abort. */
        register u64 addr __asm__("x27") = FatalErrorContext::StdAbortMagicAddress;
        register u64 val __asm__("x28")  = FatalErrorContext::StdAbortMagicValue;
        while (true) {
            __asm__ __volatile__ (
                "str %[val], [%[addr]]"
                :
                : [val]"r"(val), [addr]"r"(addr)
            );
        }
        __builtin_unreachable();
    }

}

extern "C" {

    /* Redefine abort to trigger these handlers. */
    void abort();

    /* Redefine C++ exception handlers. Requires wrap linker flag. */
    #define WRAP_ABORT_FUNC(func) void NORETURN __wrap_##func(void) { ::ams::AbortImpl(); __builtin_unreachable(); }
    WRAP_ABORT_FUNC(__cxa_pure_virtual)
    #undef WRAP_ABORT_FUNC

    void NORETURN __wrap_exit(int rc) { ::ams::Exit(rc); __builtin_unreachable(); }

}

/* Custom abort handler, so that std::abort will trigger these. */
void abort() {
    ams::AbortImpl();
    __builtin_unreachable();
}
