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

#include "hvisor_traps_single_step.hpp"

#include "../hvisor_core_context.hpp"
#include "../cpu/hvisor_cpu_sysreg_general.hpp"
#include "../cpu/hvisor_cpu_instructions.hpp"

#include "../debug_manager.h"

namespace ams::hvisor::traps {

    SingleStepState GetNextSingleStepState(ExceptionStackFrame *frame)
    {
        u64 mdscr = THERMOSPHERE_GET_SYSREG(mdscr_el1);
        bool mdscrSS = (mdscr & cpu::MDSCR_SS) != 0;
        bool pstateSS = (frame->spsr_el2 & cpu::PSR_SS) != 0;

        if (!mdscrSS) {
            return SingleStepState::Inactive;
        } else {
            return pstateSS ? SingleStepState::ActiveNotPending : SingleStepState::ActivePending;
        }

    }

    void SetNextSingleStepState(ExceptionStackFrame *frame, SingleStepState state)
    {
        u64 mdscr = THERMOSPHERE_GET_SYSREG(mdscr_el1);

        switch (state) {
            case SingleStepState::Inactive:
                // Unset mdscr_el1.ss
                mdscr &= ~cpu::MDSCR_SS;
                break;
            case SingleStepState::ActiveNotPending:
                // Set mdscr_el1.ss and pstate.ss
                mdscr |= cpu::MDSCR_SS;
                frame->spsr_el2 |= cpu::PSR_SS;
                break;
            case SingleStepState::ActivePending:
                // We never use this because pstate.ss is 0 by default...
                // Set mdscr_el1.ss and unset pstate.ss
                mdscr |= cpu::MDSCR_SS;
                frame->spsr_el2 &= cpu::PSR_SS;
                break;
            default:
                break;
        }

        THERMOSPHERE_SET_SYSREG(mdscr_el1, mdscr);
        cpu::isb(); // TRM-mandated
    }

    void HandleSingleStep(ExceptionStackFrame *frame, cpu::ExceptionSyndromeRegister esr)
    {
        uintptr_t addr = frame->elr_el2;

        // Stepping range support;
        auto [startAddr, endAddr] = currentCoreCtx->GetSteppingRange();
        if (addr >= startAddr && addr < endAddr) {
            // Reactivate single-step
            SetNextSingleStepState(frame, SingleStepState::ActiveNotPending);
        } else {
            // Disable single-step
            SetNextSingleStepState(frame, SingleStepState::Inactive);
            debugManagerReportEvent(DBGEVENT_EXCEPTION);
        }

        DEBUG("Single-step exeception ELR = 0x%016llx, ISV = %u, EX = %u\n", frame->elr_el2, (esr.iss >> 24) & 1, (esr.iss >> 6) & 1);
    }

}
