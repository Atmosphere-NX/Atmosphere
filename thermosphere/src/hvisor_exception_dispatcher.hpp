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

#include "hvisor_exception_stack_frame.hpp"
#include "cpu/hvisor_cpu_exception_sysregs.hpp"

namespace ams::hvisor {

    void EnableGeneralTraps(void);

    void DumpStackFrame(const ExceptionStackFrame *frame, bool sameEl);

    // Called on exception entry (avoids overflowing a vector section)
    void ExceptionEntryPostprocess(ExceptionStackFrame *frame, bool isLowerEl);

    // Called on exception return (avoids overflowing a vector section)
    void ExceptionReturnPreprocess(ExceptionStackFrame *frame);

    void HandleLowerElSyncException(ExceptionStackFrame *frame);

    void HandleSameElSyncException(ExceptionStackFrame *frame);

    void HandleUnknownException(u32 offset);

}
