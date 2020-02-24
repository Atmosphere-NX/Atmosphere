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

#include "../hvisor_exception_stack_frame.hpp"

namespace ams::hvisor::traps {

    enum class SingleStepState {
        Inactive            = 0, /// Single step disabled OR in the debugger
        ActiveNotPending    = 1, /// Instruction not yet executed
        ActivePending       = 2, /// Instruction executed or return-from-trap, single-step exception is going to be generated soon
    };

    /// Get the single-step state machine state (state after eret)
    SingleStepState GetNextSingleStepState(ExceptionStackFrame *frame);

    /// Set the single-step state machine state (state after eret). Frame can be nullptr iff new state is "inactive"
    void SetNextSingleStepState(ExceptionStackFrame *frame, SingleStepState state);

    void HandleSingleStep(ExceptionStackFrame *frame, cpu::ExceptionSyndromeRegister esr);

}