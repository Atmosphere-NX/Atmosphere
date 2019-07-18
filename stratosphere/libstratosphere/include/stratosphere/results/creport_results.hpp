/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>

static constexpr u32 Module_Creport = 168;

static constexpr Result ResultCreportUndefinedInstruction = MAKERESULT(Module_Creport, 0);
static constexpr Result ResultCreportInstructionAbort     = MAKERESULT(Module_Creport, 1);
static constexpr Result ResultCreportDataAbort            = MAKERESULT(Module_Creport, 2);
static constexpr Result ResultCreportAlignmentFault       = MAKERESULT(Module_Creport, 3);
static constexpr Result ResultCreportDebuggerAttached     = MAKERESULT(Module_Creport, 4);
static constexpr Result ResultCreportBreakPoint           = MAKERESULT(Module_Creport, 5);
static constexpr Result ResultCreportUserBreak            = MAKERESULT(Module_Creport, 6);
static constexpr Result ResultCreportDebuggerBreak        = MAKERESULT(Module_Creport, 7);
static constexpr Result ResultCreportUndefinedSystemCall  = MAKERESULT(Module_Creport, 8);
static constexpr Result ResultCreportSystemMemoryError    = MAKERESULT(Module_Creport, 9);

static constexpr Result ResultCreportIncompleteReport     = MAKERESULT(Module_Creport, 99);
