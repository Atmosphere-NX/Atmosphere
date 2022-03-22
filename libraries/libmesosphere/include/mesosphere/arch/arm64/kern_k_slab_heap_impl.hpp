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
#pragma once
#include <vapours.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_select_interrupt_manager.hpp>

namespace ams::kern::arch::arm64 {

    template<typename T>
    concept SlabHeapNode = requires (T &t) {
        { t.next } -> std::convertible_to<T *>;
    };

    ALWAYS_INLINE bool IsSlabAtomicValid() {
        /* Without careful consideration, slab heaps atomics are vulnerable to */
        /* the ABA problem, when doing compare and swap of node pointers. */
        /* We resolve this by using the ARM exclusive monitor; we bundle the */
        /* load and store of the relevant values into a single exclusive monitor */
        /* hold, preventing the ABA problem. */
        /* However, our assembly must do both a load and a store under a single */
        /* hold, at different memory addresses. Considering the case where the */
        /* addresses are distinct but resolve to the same cache set (by chance), */
        /* we can note that under a 1-way associative (direct-mapped) cache */
        /* we would have as a guarantee that the second access would evict the */
        /* cache line from the first access, invalidating our exclusive monitor */
        /* hold. Thus, we require that the cache is not 1-way associative, for */
        /* our implementation to be correct. */
        {
            /* Disable interrupts. */
            KScopedInterruptDisable di;

            /* Select L1 cache. */
            cpu::SetCsselrEl1(0);
            cpu::InstructionMemoryBarrier();

            /* Check that the L1 cache is not direct-mapped. */
            return cpu::CacheSizeIdRegisterAccessor().GetAssociativity() != 0;
        }
    }

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
            "2:\n"
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
            : [tmp]"=&r"(tmp), [node]"+&r"(node), [next]"=&r"(next), [head]"+&r"(head)
            :
            : "cc", "memory"
        );
    }

}
