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
#include <mesosphere/kern_common.hpp>

namespace ams::kern::arch::arm64 {

    struct KExceptionContext {
        u64 x[(30 - 0) + 1];
        u64 sp;
        u64 pc;
        u32 psr;
        u32 write;
        u64 tpidr;
        u64 reserved;

        constexpr void GetSvcThreadContext(ams::svc::LastThreadContext *out) const {
            if ((this->psr & 0x10) == 0) {
                /* aarch64 thread. */
                out->fp = this->x[29];
                out->sp = this->sp;
                out->lr = this->x[30];
                out->pc = this->pc;
            } else {
                /* aarch32 thread. */
                out->fp = this->x[11];
                out->sp = this->x[13];
                out->lr = this->x[14];
                out->pc = this->pc;
            }
        }
    };
    static_assert(sizeof(KExceptionContext) == EXCEPTION_CONTEXT_SIZE);

    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 0]) == EXCEPTION_CONTEXT_X0);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 1]) == EXCEPTION_CONTEXT_X1);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 2]) == EXCEPTION_CONTEXT_X2);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 3]) == EXCEPTION_CONTEXT_X3);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 4]) == EXCEPTION_CONTEXT_X4);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 5]) == EXCEPTION_CONTEXT_X5);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 6]) == EXCEPTION_CONTEXT_X6);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 7]) == EXCEPTION_CONTEXT_X7);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 8]) == EXCEPTION_CONTEXT_X8);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[ 9]) == EXCEPTION_CONTEXT_X9);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[10]) == EXCEPTION_CONTEXT_X10);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[11]) == EXCEPTION_CONTEXT_X11);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[12]) == EXCEPTION_CONTEXT_X12);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[13]) == EXCEPTION_CONTEXT_X13);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[14]) == EXCEPTION_CONTEXT_X14);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[15]) == EXCEPTION_CONTEXT_X15);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[16]) == EXCEPTION_CONTEXT_X16);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[17]) == EXCEPTION_CONTEXT_X17);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[18]) == EXCEPTION_CONTEXT_X18);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[19]) == EXCEPTION_CONTEXT_X19);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[20]) == EXCEPTION_CONTEXT_X20);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[21]) == EXCEPTION_CONTEXT_X21);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[22]) == EXCEPTION_CONTEXT_X22);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[23]) == EXCEPTION_CONTEXT_X23);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[24]) == EXCEPTION_CONTEXT_X24);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[25]) == EXCEPTION_CONTEXT_X25);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[26]) == EXCEPTION_CONTEXT_X26);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[27]) == EXCEPTION_CONTEXT_X27);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[28]) == EXCEPTION_CONTEXT_X28);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[29]) == EXCEPTION_CONTEXT_X29);
    static_assert(AMS_OFFSETOF(KExceptionContext, x[30]) == EXCEPTION_CONTEXT_X30);
    static_assert(AMS_OFFSETOF(KExceptionContext, sp)    == EXCEPTION_CONTEXT_SP);
    static_assert(AMS_OFFSETOF(KExceptionContext, pc)    == EXCEPTION_CONTEXT_PC);
    static_assert(AMS_OFFSETOF(KExceptionContext, psr)   == EXCEPTION_CONTEXT_PSR);
    static_assert(AMS_OFFSETOF(KExceptionContext, tpidr) == EXCEPTION_CONTEXT_TPIDR);

}