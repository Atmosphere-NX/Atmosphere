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

typedef struct DataAbortIss {
    u32 dfsc        : 6; // Fault status code

    u32 wnr         : 1; // Write, not Read
    u32 s1ptw       : 1; // Stage1 page table walk fault
    u32 cm          : 1; // Cache maintenance
    u32 ea          : 1; // External abort
    u32 fnv         : 1; // FAR not Valid
    u32 set         : 2; // Synchronous error type
    u32 vncr        : 1; // vncr_el2 trap

    u32 ar          : 1; // Acquire/release. Bit 14
    u32 sf          : 1; // 64-bit register used
    u32 srt         : 5; // Syndrome register transfer (register used)
    u32 sse         : 1; // Syndrome sign extend
    u32 sas         : 2; // Syndrome access size. Bit 23

    u32 isv         : 1; // Instruction syndrome valid (ISS[23:14] valid)
} DataAbortIss;

void dumpUnhandledDataAbort(DataAbortIss dabtIss, u64 far, const char *msg);
void handleLowerElDataAbortException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr);
