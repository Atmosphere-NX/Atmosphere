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
#include <assert.h>
#include "utils.h"

typedef struct ExceptionStackFrame {
    u64 x[31]; // x0 .. x30
    u64 sp_el1;
    union {
        u64 sp_el2;
        u64 sp_el0;
    };
    u64 elr_el2;
    u64 spsr_el2;
    u64 far_el2;
    u64 cntpct_el0;
    u64 cntp_ctl_el0;
    u64 cntv_ctl_el0;
    u64 reserved;
} ExceptionStackFrame;

static_assert(sizeof(ExceptionStackFrame) == 0x140, "Wrong size for ExceptionStackFrame");

// Adapted from https://developer.arm.com/docs/ddi0596/a/a64-shared-pseudocode-functions/shared-exceptions-pseudocode
typedef enum ExceptionClass {
    Exception_Uncategorized = 0x0,
    Exception_WFxTrap = 0x1,
    Exception_CP15RTTrap = 0x3,
    Exception_CP15RRTTrap = 0x4,
    Exception_CP14RTTrap = 0x5,
    Exception_CP14DTTrap = 0x6,
    Exception_AdvSIMDFPAccessTrap = 0x7,
    Exception_FPIDTrap = 0x8,
    Exception_PACTrap = 0x9,
    Exception_CP14RRTTrap = 0xC,
    Exception_BranchTargetException = 0xD, // No official enum field name from Arm yet
    Exception_IllegalState = 0xE,
    Exception_SupervisorCallA32 = 0x11,
    Exception_HypervisorCallA32 = 0x12,
    Exception_MonitorCallA32 = 0x13,
    Exception_SupervisorCallA64 = 0x15,
    Exception_HypervisorCallA64 = 0x16,
    Exception_MonitorCallA64 = 0x17,
    Exception_SystemRegisterTrap = 0x18,
    Exception_SVEAccessTrap = 0x19,
    Exception_ERetTrap = 0x1A,
    Exception_El3_ImplementationDefined = 0x1F,
    Exception_InstructionAbortLowerEl = 0x20,
    Exception_InstructionAbortSameEl = 0x21,
    Exception_PCAlignment = 0x22,
    Exception_DataAbortLowerEl = 0x24,
    Exception_DataAbortSameEl = 0x25,
    Exception_SPAlignment = 0x26,
    Exception_FPTrappedExceptionA32 = 0x28,
    Exception_FPTrappedExceptionA64 = 0x2C,
    Exception_SError = 0x2F,
    Exception_BreakpointLowerEl = 0x30,
    Exception_BreakpointSameEl = 0x31,
    Exception_SoftwareStepLowerEl = 0x32,
    Exception_SoftwareStepSameEl = 0x33,
    Exception_WatchpointLowerEl = 0x34,
    Exception_WatchpointSameEl = 0x35,
    Exception_SoftwareBreakpointA32 = 0x38,
    Exception_VectorCatchA32 = 0x3A,
    Exception_SoftwareBreakpointA64 = 0x3C,
} ExceptionClass;

typedef struct ExceptionSyndromeRegister {
    u32 iss             : 25;   // Instruction Specific Syndrome
    u32 il              :  1;   // Instruction Length (16 or 32-bit)
    ExceptionClass ec   :  6;   // Exception Class
    u32 res0            : 32;
} ExceptionSyndromeRegister;

static inline bool spsrIsA32(u64 spsr)
{
    return (spsr & 0x10) != 0;
}

static inline bool spsrIsThumb(u64 spsr)
{
    return spsrIsA32(spsr) && (spsr & 0x20) != 0;
}

static inline u32 spsrGetT32ItFlags(u64 spsr)
{
    return (((spsr >> 10) & 0x3F) << 2) | ((spsr >> 25) & 3);
}

static inline void spsrSetT32ItFlags(u64 *spsr, u32 itFlags)
{
    static const u32 itMask = (0x3F << 10) | (3 << 25);
    *spsr &= ~itMask;
    *spsr |= (itFlags & 3) << 25;
    *spsr |= ((itFlags >> 2) & 0x3F) << 10;
}

static inline u64 readFrameRegister(ExceptionStackFrame *frame, u32 id)
{
    return frame->x[id];
}

static inline u64 readFrameRegisterZ(ExceptionStackFrame *frame, u32 id)
{
    return id == 31 ? 0 /* xzr */ : frame->x[id];
}

static inline void writeFrameRegister(ExceptionStackFrame *frame, u32 id, u64 val)
{
    frame->x[id] = val;
}

static inline void writeFrameRegisterZ(ExceptionStackFrame *frame, u32 id, u64 val)
{
    if (id != 31) {
        // If not xzr
        frame->x[id] = val;
    }
}

bool spsrEvaluateConditionCode(u64 spsr, u32 conditionCode);
void skipFaultingInstruction(ExceptionStackFrame *frame, u32 size);
void dumpStackFrame(const ExceptionStackFrame *frame, bool sameEl);

void exceptionEnterInterruptibleHypervisorCode(ExceptionStackFrame *frame);

void handleLowerElSyncException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr);
void handleSameElSyncException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr);
void handleUnknownException(u32 offset);
