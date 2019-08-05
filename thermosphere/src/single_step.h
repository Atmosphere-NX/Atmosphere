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

#include "utils.h"
#include "exceptions.h"

typedef enum SingleStepState {
    SingleStepState_Inactive            = 0, // Single step disabled OR in the debugger
    SingleStepState_ActivePending       = 1, // Instruction not yet executed
    SingleStepState_ActiveNotPending    = 2, // Instruction executed, single-step exception is going to be generated soon
} SingleStepState;

/// Get the single-step state machine state (state after eret)
SingleStepState singleStepGetNextState(ExceptionStackFrame *frame);

/// Set the single-step state machine state (state after eret). Frame can be NULL iff new state is "inactive"
void singleStepSetNextState(ExceptionStackFrame *frame, SingleStepState state);

void handleSingleStep(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr);
