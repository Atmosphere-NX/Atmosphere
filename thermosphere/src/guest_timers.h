/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "exceptions.h"
#include "sysreg.h"

static inline u64 computeCntvct(const ExceptionStackFrame *frame)
{
    return frame->cntpct_el0 - currentCoreCtx->totalTimeInHypervisor;
}

static inline void writeEmulatedPhysicalCompareValue(ExceptionStackFrame *frame, u64 val)
{
    // We lied about the value of cntpct, so we need to compute the time delta
    // the guest actually intended to use...
    u64 vct = computeCntvct(frame);
    currentCoreCtx->emulPtimerCval = val;
    SET_SYSREG(cntp_cval_el0, frame->cntpct_el0 + (val - vct));
}
