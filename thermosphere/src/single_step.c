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

SingleStepState singleStepGetNextState(ExceptionStackFrame *frame)
{
    u64 mdscr = GET_SYSREG(mdscr_el1);
    bool mdscrSS = (mdscr & MDSCR_EL1_SS) != 0;
    bool pstateSS = (frame->spsr_el2 & PSTATE_SS) != 0;

    if (!mdscrSS) {
        return SingleStepState_Inactive;
    } else {
        return pstateSS ? SingleStepState_ActivePending : SingleStepState_ActiveNotPending;
    }
}

void singleStepSetNextState(ExceptionStackFrame *frame, SingleStepState state)
{
    u64 mdscr = GET_SYSREG(mdscr_el1);

    switch (state) {
        case SingleStepState_Inactive:
            // Unset mdscr_el1.ss
            mdscr &= ~MDSCR_EL1_SS;
            break;
        case SingleStepState_ActivePending:
            // Set mdscr_el1.ss and pstate.ss
            mdscr |= MDSCR_EL1_SS;
            frame->spsr_el2 |= PSTATE_SS;
            break;
        case SingleStepState_ActiveNotPending:
            // Set mdscr_el1.ss and unset pstate.ss
            mdscr |= MDSCR_EL1_SS;
            frame->spsr_el2 |= PSTATE_SS;
            break;
        default:
            break;
    }

    SET_SYSREG(mdscr_el1, mdscr);
}

void handleSingleStep(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    // Disable single-step ASAP
    singleStepSetNextState(NULL, SingleStepState_Inactive);

    DEBUG("Single-step exeception ELR = 0x%016llx, ISV = %u, EX = %u\n", frame->elr_el2, (esr.iss >> 24) & 1, (esr.iss >> 6) & 1);

    // Hehe boi
    //singleStepSetNextState(frame, SingleStepState_ActivePending);
}
