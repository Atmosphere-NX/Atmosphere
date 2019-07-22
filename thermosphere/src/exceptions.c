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

#include "hvc.h"
#include "log.h"

void dumpStackFrame(const ExceptionStackFrame *frame, bool sameEl)
{
#ifndef NDEBUG
    for (u32 i = 0; i < 30; i += 2) {
        serialLog("x%u\t\t%08llx\t\tx%u\t\t%08llx\r\n", i, frame->x[i], i + 1, frame->x[i + 1]);
    }

    serialLog("x30\t\t%08llx\r\n\r\n", frame->x[30]);
    serialLog("elr_el2\t\t%08llx\r\n", frame->elr_el2);
    serialLog("spsr_el2\t%08llx\r\n", frame->spsr_el2);
    if (sameEl) {
        serialLog("sp_el2\t\t%08llx\r\n", frame->sp_el2);
    } else {
        serialLog("sp_el0\t\t%08llx\r\n", frame->sp_el0);
    }
    serialLog("sp_el1\t\t%08llx\r\n", frame->sp_el1);
#endif
}

void handleLowerElSyncException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{

    switch (esr.ec) {
        case Exception_HypervisorCallA64:
        case Exception_HypervisorCallA32:
            handleHypercall(frame, esr);
            break;
        default:
            serialLog("Lower EL sync exception, EC = 0x%02llx IL=%llu ISS=0x%06llx\r\n", (u64)esr.ec, esr.il, esr.iss);
            dumpStackFrame(frame, false);
            break;
    }
}

void handleSameElSyncException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    serialLog("Same EL sync exception, EC = 0x%02llx IL=%llu ISS=0x%06llx\r\n", (u64)esr.ec, esr.il, esr.iss);
    dumpStackFrame(frame, true);
}

void handleUnknownException(u32 offset)
{
    serialLog("Unknown exception! (offset 0x%03lx)\r\n", offset);
}
