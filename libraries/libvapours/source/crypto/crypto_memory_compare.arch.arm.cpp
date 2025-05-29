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
#include <vapours.hpp>

namespace ams::crypto {

    bool IsSameBytes(const void *lhs, const void *rhs, size_t size) {
        bool result;
        u8 xor_acc, ltmp, rtmp;
        size_t index;

        __asm__ __volatile__(
            /* Clear registers and prepare for comparison. */
            "   movs     %[xor_acc], #0\n"
            "   movs     %[index], #0\n"
            "   b       1f\n"

            /* Compare one byte in constant time. */
            "0:\n"
            "   ldrb    %[ltmp], [%[lhs]]\n"
            "   ldrb    %[rtmp], [%[rhs]]\n"
            "   adds    %[lhs], #1\n"
            "   adds    %[rhs], #1\n"
            "   eors    %[ltmp], %[ltmp], %[rtmp]\n"
            "   orrs    %[xor_acc], %[xor_acc], %[ltmp]\n"
            "   adds    %[index], #1\n"

            /* Check if there is still data to compare. */
            "1:\n"
            "   cmp     %[index], %[size]\n"
            "   bcc     0b\n"

            /* We're done, set result. */
            "   cmp     %[xor_acc], #0\n"
            "   moveq   %[result],  #1\n"
            "   movne   %[result],  #0\n"
            : [result]"=r"(result), [lhs]"+r"(lhs), [rhs]"+r"(rhs), [xor_acc]"=&r"(xor_acc), [index]"=&r"(index), [ltmp]"=&r"(ltmp), [rtmp]"=&r"(rtmp)
            : "m"(*(const u8 (*)[size])lhs), "m"(*(const u8 (*)[size])rhs), [size]"r"(size)
            : "cc"
        );

        return result;
    }

}