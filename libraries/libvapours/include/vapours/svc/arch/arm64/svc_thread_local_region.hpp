/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License
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
#include <vapours/svc/svc_types_common.hpp>

namespace ams::svc::arch::arm64 {

    constexpr inline size_t NumTlsSlots = 16;
    constexpr inline size_t MessageBufferSize = 0x100;

    struct ThreadLocalRegion {
        u32 message_buffer[MessageBufferSize / sizeof(u32)];
        volatile u16 disable_count;
        volatile u16 interrupt_flag;
        /* TODO: How should we handle libnx vs Nintendo user thread local space? */
        uintptr_t TODO[(0x200 - 0x108) / sizeof(uintptr_t)];
    };

    ALWAYS_INLINE ThreadLocalRegion *GetThreadLocalRegion() {
        ThreadLocalRegion *tlr;
        __asm__ __volatile__("mrs %[tlr], tpidrro_el0" : [tlr]"=&r"(tlr) :: "memory");
        return tlr;
    }

}
