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
            u32 m_packed_tickets;
        public:
            constexpr KNotAlignedSpinLock() : m_packed_tickets(0) { /* ... */ }

            ALWAYS_INLINE void Lock() {
                u32 tmp0, tmp1, tmp2;

                __asm__ __volatile__(
                    "    prfm   pstl1keep, %[m_packed_tickets]\n"
                    "1:\n"
                    "    ldaxr  %w[tmp0], %[m_packed_tickets]\n"
                    "    add    %w[tmp2], %w[tmp0], #0x10000\n"
                    "    stxr   %w[tmp1], %w[tmp2], %[m_packed_tickets]\n"
                    "    cbnz   %w[tmp1], 1b\n"
                    "    \n"
                    "    and    %w[tmp1], %w[tmp0], #0xFFFF\n"
                    "    cmp    %w[tmp1], %w[tmp0], lsr #16\n"
                    "    b.eq   3f\n"
                    "    sevl\n"
                    "2:\n"
                    "    wfe\n"
                    "    ldaxrh %w[tmp1], %[m_packed_tickets]\n"
                    "    cmp    %w[tmp1], %w[tmp0], lsr #16\n"
                    "    b.ne   2b\n"
                    "3:\n"
                    : [tmp0]"=&r"(tmp0), [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [m_packed_tickets]"+Q"(m_packed_tickets)
                    :
                    : "cc", "memory"
                );
            }

            ALWAYS_INLINE void Unlock() {
                const u32 value = m_packed_tickets + 1;
                __asm__ __volatile__(
                    "    stlrh %w[value], %[m_packed_tickets]\n"
                    : [m_packed_tickets]"+Q"(m_packed_tickets)
                    : [value]"r"(value)
                    : "memory"
                );
            }
    };
    static_assert(sizeof(KNotAlignedSpinLock) == sizeof(u32));

    class KAlignedSpinLock {
        private:
            alignas(cpu::DataCacheLineSize) u16 m_current_ticket;
            alignas(cpu::DataCacheLineSize) u16 m_next_ticket;
        public:
            constexpr KAlignedSpinLock() : m_current_ticket(0), m_next_ticket(0) { /* ... */ }

            ALWAYS_INLINE void Lock() {
                u32 tmp0, tmp1, got_lock;

                __asm__ __volatile__(
                    "    prfm   pstl1keep, %[m_next_ticket]\n"
                    "1:\n"
                    "    ldaxrh %w[tmp0], %[m_next_ticket]\n"
                    "    add    %w[tmp1], %w[tmp0], #0x1\n"
                    "    stxrh  %w[got_lock], %w[tmp1], %[m_next_ticket]\n"
                    "    cbnz   %w[got_lock], 1b\n"
                    "    \n"
                    "    sevl\n"
                    "2:\n"
                    "    wfe\n"
                    "    ldaxrh %w[tmp1], %[m_current_ticket]\n"
                    "    cmp    %w[tmp1], %w[tmp0]\n"
                    "    b.ne   2b\n"
                    : [tmp0]"=&r"(tmp0), [tmp1]"=&r"(tmp1), [got_lock]"=&r"(got_lock), [m_next_ticket]"+Q"(m_next_ticket)
                    : [m_current_ticket]"Q"(m_current_ticket)
                    : "cc", "memory"
                );
            }

            ALWAYS_INLINE void Unlock() {
                const u32 value = m_current_ticket + 1;
                __asm__ __volatile__(
                    "    stlrh %w[value], %[m_current_ticket]\n"
                    : [m_current_ticket]"+Q"(m_current_ticket)
                    : [value]"r"(value)
                    : "memory"
                );
            }
    };
    static_assert(sizeof(KAlignedSpinLock) == 2 * cpu::DataCacheLineSize);

    using KSpinLock = KNotAlignedSpinLock;

}
