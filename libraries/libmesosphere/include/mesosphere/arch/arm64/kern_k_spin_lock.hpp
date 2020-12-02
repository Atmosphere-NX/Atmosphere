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
#pragma once
#include <vapours.hpp>
#include <mesosphere/kern_select_cpu.hpp>

namespace ams::kern::arch::arm64 {

    class KNotAlignedSpinLock {
        private:
            u32 packed_tickets;
        public:
            constexpr KNotAlignedSpinLock() : packed_tickets(0) { /* ... */ }

            ALWAYS_INLINE void Lock() {
                u32 tmp0, tmp1, tmp2;

                __asm__ __volatile__(
                    "    prfm   pstl1keep, %[packed_tickets]\n"
                    "1:\n"
                    "    ldaxr  %w[tmp0], %[packed_tickets]\n"
                    "    add    %w[tmp2], %w[tmp0], #0x10000\n"
                    "    stxr   %w[tmp1], %w[tmp2], %[packed_tickets]\n"
                    "    cbnz   %w[tmp1], 1b\n"
                    "    \n"
                    "    and    %w[tmp1], %w[tmp0], #0xFFFF\n"
                    "    cmp    %w[tmp1], %w[tmp0], lsr #16\n"
                    "    b.eq   3f\n"
                    "    sevl\n"
                    "2:\n"
                    "    wfe\n"
                    "    ldaxrh %w[tmp1], %[packed_tickets]\n"
                    "    cmp    %w[tmp1], %w[tmp0], lsr #16\n"
                    "    b.ne   2b\n"
                    "3:\n"
                    : [tmp0]"=&r"(tmp0), [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [packed_tickets]"+Q"(this->packed_tickets)
                    :
                    : "cc", "memory"
                );
            }

            ALWAYS_INLINE void Unlock() {
                const u32 value = this->packed_tickets + 1;
                __asm__ __volatile__(
                    "    stlrh %w[value], %[packed_tickets]\n"
                    : [packed_tickets]"+Q"(this->packed_tickets)
                    : [value]"r"(value)
                    : "memory"
                );
            }
    };
    static_assert(sizeof(KNotAlignedSpinLock) == sizeof(u32));

    class KAlignedSpinLock {
        private:
            alignas(cpu::DataCacheLineSize) u16 current_ticket;
            alignas(cpu::DataCacheLineSize) u16 next_ticket;
        public:
            constexpr KAlignedSpinLock() : current_ticket(0), next_ticket(0) { /* ... */ }

            ALWAYS_INLINE void Lock() {
                u32 tmp0, tmp1, got_lock;

                __asm__ __volatile__(
                    "    prfm   pstl1keep, %[next_ticket]\n"
                    "1:\n"
                    "    ldaxrh %w[tmp0], %[next_ticket]\n"
                    "    add    %w[tmp1], %w[tmp0], #0x1\n"
                    "    stxrh  %w[got_lock], %w[tmp1], %[next_ticket]\n"
                    "    cbnz   %w[got_lock], 1b\n"
                    "    \n"
                    "    sevl\n"
                    "2:\n"
                    "    wfe\n"
                    "    ldaxrh %w[tmp1], %[current_ticket]\n"
                    "    cmp    %w[tmp1], %w[tmp0]\n"
                    "    b.ne   2b\n"
                    : [tmp0]"=&r"(tmp0), [tmp1]"=&r"(tmp1), [got_lock]"=&r"(got_lock), [next_ticket]"+Q"(this->next_ticket)
                    : [current_ticket]"Q"(this->current_ticket)
                    : "cc", "memory"
                );
            }

            ALWAYS_INLINE void Unlock() {
                const u32 value = this->current_ticket + 1;
                __asm__ __volatile__(
                    "    stlrh %w[value], %[current_ticket]\n"
                    : [current_ticket]"+Q"(this->current_ticket)
                    : [value]"r"(value)
                    : "memory"
                );
            }
    };
    static_assert(sizeof(KAlignedSpinLock) == 2 * cpu::DataCacheLineSize);

    using KSpinLock = KNotAlignedSpinLock;

}
