/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "../defines.hpp"

#define _ASM_ARITHMETIC_UNARY_HELPER(sz, regalloc, op) ({\
    u##sz res;\
    __asm__ __volatile__ (STRINGIZE(op) " %" STRINGIZE(regalloc) "[res], %" STRINGIZE(regalloc) "[val]" : [res] "=r" (res) : [val] "r" (val));\
    res;\
})

#define DECLARE_SINGLE_ASM_INSN2(name, what) static inline void name() { __asm__ __volatile__ (what ::: "memory"); }
#define DECLARE_SINGLE_ASM_INSN(name) static inline void name() { __asm__ __volatile__ (STRINGIZE(name) ::: "memory"); }

namespace ams::hvisor::cpu {


    template<typename T>
    ALWAYS_INLINE static T rbit(T val)
    {
        static_assert(std::is_integral_v<T> && (sizeof(T) == 8 || sizeof(T) == 4));
        if constexpr (sizeof(T) == 8) {
            return _ASM_ARITHMETIC_UNARY_HELPER(64, x, rbit);
        } else {
            return _ASM_ARITHMETIC_UNARY_HELPER(32, w, rbit);
        }
    }

    DECLARE_SINGLE_ASM_INSN(wfi)
    DECLARE_SINGLE_ASM_INSN(wfe)
    DECLARE_SINGLE_ASM_INSN(sevl)
    DECLARE_SINGLE_ASM_INSN(sev)
    DECLARE_SINGLE_ASM_INSN2(dmb, "dmb ish")
    DECLARE_SINGLE_ASM_INSN2(dmbSy, "dmb sy")
    DECLARE_SINGLE_ASM_INSN2(dsb, "dsb ish")
    DECLARE_SINGLE_ASM_INSN2(dsbSy, "dsb sy")
    DECLARE_SINGLE_ASM_INSN(isb)

    DECLARE_SINGLE_ASM_INSN2(TlbInvalidateEl2, "tlbi alle2is")
    DECLARE_SINGLE_ASM_INSN2(TlbInvalidateEl1, "tlbi vmalle1is")
    DECLARE_SINGLE_ASM_INSN2(TlbInvalidateEl1Stage12, "tlbi alle1is")

    ALWAYS_INLINE static void TlbInvalidateEl2Page(uintptr_t addr)
    {
        __asm__ __volatile__ ("tlbi vae2is, %0" :: "r"(addr) : "memory");
    }

}

#undef DECLARE_SINGLE_ASM_INSN
#undef DECLARE_SINGLE_ASM_INSN2
#undef _ASM_ARITHMETIC_UNARY_HELPER
