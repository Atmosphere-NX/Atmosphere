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
#include <vapours/results/results_common.hpp>

namespace ams::creport {

    R_DEFINE_NAMESPACE_RESULT_MODULE(168);

    R_DEFINE_ERROR_RESULT(UndefinedInstruction, 0);
    R_DEFINE_ERROR_RESULT(InstructionAbort,     1);
    R_DEFINE_ERROR_RESULT(DataAbort,            2);
    R_DEFINE_ERROR_RESULT(AlignmentFault,       3);
    R_DEFINE_ERROR_RESULT(DebuggerAttached,     4);
    R_DEFINE_ERROR_RESULT(BreakPoint,           5);
    R_DEFINE_ERROR_RESULT(UserBreak,            6);
    R_DEFINE_ERROR_RESULT(DebuggerBreak,        7);
    R_DEFINE_ERROR_RESULT(UndefinedSystemCall,  8);
    R_DEFINE_ERROR_RESULT(MemorySystemError,    9);

    R_DEFINE_ERROR_RESULT(IncompleteReport,     99);

}
