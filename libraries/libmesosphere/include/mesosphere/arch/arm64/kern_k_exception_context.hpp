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
    static_assert(sizeof(KExceptionContext) == 0x120);

}