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

    template<typename T>
    concept SlabHeapNode = requires (T &t) {
        { t.next } -> std::convertible_to<T *>;
    };

    template<typename T> requires SlabHeapNode<T>
    ALWAYS_INLINE T *AllocateFromSlabAtomic(T **head) {
        u32 tmp;
        T *node, *next;

        __asm__ __volatile__(
            "1:\n"
            "    ldaxr  %[node], [%[head]]\n"
            "    cbz    %[node], 2f\n"
            "    ldr    %[next], [%[node]]\n"
            "    stlxr  %w[tmp], %[next], [%[head]]\n"
            "    cbnz   %w[tmp], 1b\n"
            "    b      3f\n"
            "2:\n"
            "    clrex\n"
            "3:\n"
            : [tmp]"=&r"(tmp), [node]"=&r"(node), [next]"=&r"(next), [head]"+&r"(head)
            :
            : "cc", "memory"
        );

        return node;
    }

    template<typename T> requires SlabHeapNode<T>
    ALWAYS_INLINE void FreeToSlabAtomic(T **head, T *node) {
        u32 tmp;
        T *next;

        __asm__ __volatile__(
            "1:\n"
            "    ldaxr  %[next], [%[head]]\n"
            "    str    %[next], [%[node]]\n"
            "    stlxr  %w[tmp], %[node], [%[head]]\n"
            "    cbnz   %w[tmp], 1b\n"
            "2:\n"
            : [tmp]"=&r"(tmp), [node]"+&r"(node), [next]"=&r"(next), [head]"+&r"(head)
            :
            : "cc", "memory"
        );
    }

}
