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

#include "single_step.h"
#include "core_ctx.h"
#include "sysreg.h"
#include "debug_log.h"

void enableSingleStepExceptions(void)
{
    u64 mdscr = GET_SYSREG(mdscr_el1);

    // Enable Single Step functionality
    mdscr |= BIT(0);

    SET_SYSREG(mdscr_el1, mdscr);
}

void setSingleStep(ExceptionStackFrame *frame, bool singleStep)
{
    // Set or clear SPSR.SS
    if (singleStep) {
        frame->spsr_el2 |= BITL(22);
    } else {
        frame->spsr_el2 &= ~BITL(22);
    }

    currentCoreCtx->wasSingleStepping = singleStep;
}

void handleSingleStep(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    DEBUG("Single-step exeception ELR = 0x%016llx, ISV = %u, EX = %u\n", frame->elr_el2, (esr.iss >> 24) & 1, (esr.iss >> 6) & 1);
    setSingleStep(frame, true); // hehe boi
}
