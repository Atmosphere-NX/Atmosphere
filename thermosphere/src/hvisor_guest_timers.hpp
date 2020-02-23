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

#include "hvisor_core_context.hpp"
#include "hvisor_exception_stack_frame.hpp"
#include "cpu/hvisor_cpu_sysreg_general.hpp"

namespace ams::hvisor {

    static inline u64 ComputeCntvct(const ExceptionStackFrame *frame)
    {
        return frame->cntpct_el0 - currentCoreCtx->GetTotalTimeInHypervisor();
    }

    static inline void WriteEmulatedPhysicalCompareValue(ExceptionStackFrame *frame, u64 val)
    {
        // We lied about the value of cntpct, so we need to compute the time delta
        // the guest actually intended to use...
        u64 vct = ComputeCntvct(frame);
        currentCoreCtx->SetEmulPtimerCval(val);
        THERMOSPHERE_SET_SYSREG(cntp_cval_el0, frame->cntpct_el0 + (val - vct));
    }

    static inline bool CheckRescheduleEmulatedPtimer(ExceptionStackFrame *frame)
    {
        // Evaluate if the timer has really expired in the PoV of the guest kernel.
        // If not, reschedule (add missed time delta) it & exit early
        u64 cval = currentCoreCtx->GetEmulPtimerCval();
        u64 vct  = ComputeCntvct(frame);

        if (cval > vct) {
            // It has not: reschedule the timer
            // Note: this isn't 100% precise esp. on QEMU so it may take a few tries...
            WriteEmulatedPhysicalCompareValue(frame, cval);
            return false;
        }

        return true;
    }

    static inline void EnableGuestTimerTraps(void)
    {
        // Disable event streams, trap everything
        u64 cnthctl = 0;
        THERMOSPHERE_SET_SYSREG(cnthctl_el2, cnthctl);
    }

}
