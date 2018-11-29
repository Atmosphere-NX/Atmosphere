/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <stratosphere.hpp>

#include "fatal_types.hpp"

void TryCollectDebugInformation(FatalThrowContext *ctx, u64 pid);

struct StackFrame {
    u64 fp;
    u64 lr;
};

struct AttachProcessInfo {
    u64 title_id;
    u64 process_id;
    char name[0xC];
    u32 flags;
    u64 user_exception_context_address; /* 5.0.0+ */
};

struct AttachThreadInfo {
    u64 thread_id;
    u64 tls_address;
    u64 entrypoint;
};

/* TODO: ExitProcessInfo */
/* TODO: ExitThreadInfo */

enum class DebugExceptionType : u32 {
    UndefinedInstruction = 0,
    InstructionAbort = 1,
    DataAbort = 2,
    AlignmentFault = 3,
    DebuggerAttached = 4,
    BreakPoint = 5,
    UserBreak = 6,
    DebuggerBreak = 7,
    BadSvc = 8,
    UnknownNine = 9,
};

static inline const char *GetDebugExceptionTypeStr(DebugExceptionType type) {
    switch (type) {
        case DebugExceptionType::UndefinedInstruction:
            return "Undefined Instruction";
        case DebugExceptionType::InstructionAbort:
            return "Instruction Abort";
        case DebugExceptionType::DataAbort:
            return "Data Abort";
        case DebugExceptionType::AlignmentFault:
            return "Alignment Fault";
        case DebugExceptionType::DebuggerAttached:
            return "Debugger Attached";
        case DebugExceptionType::BreakPoint:
            return "Break Point";
        case DebugExceptionType::UserBreak:
            return "User Break";
        case DebugExceptionType::DebuggerBreak:
            return "Debugger Break";
        case DebugExceptionType::BadSvc:
            return "Bad Svc";
        case DebugExceptionType::UnknownNine:
            return "Unknown Nine";
        default:
            return "Unknown";
    }
}

struct UndefinedInstructionInfo {
    u32 insn;
};

struct DataAbortInfo {
    u64 address;
};

struct AlignmentFaultInfo {
    u64 address;
};

struct UserBreakInfo {
    u64 break_reason;
    u64 address;
    u64 size;
};

struct BadSvcInfo {
    u32 id;
};

union SpecificExceptionInfo {
    UndefinedInstructionInfo undefined_instruction;
    DataAbortInfo data_abort;
    AlignmentFaultInfo alignment_fault;
    UserBreakInfo user_break;
    BadSvcInfo bad_svc;
    u64 raw;
};

struct ExceptionInfo {
    DebugExceptionType type;
    u64 address;
    SpecificExceptionInfo specific;
};


enum class DebugEventType : u32 {
    AttachProcess = 0,
    AttachThread = 1,
    ExitProcess = 2,
    ExitThread = 3,
    Exception = 4
};

union DebugInfo {
    AttachProcessInfo attach_process;
    AttachThreadInfo attach_thread;
    ExceptionInfo exception;
};

struct DebugEventInfo {
    DebugEventType type;
    u32 flags;
    u64 thread_id;
    union {
        DebugInfo info;
        u64 _[0x40/sizeof(u64)];
    };
};

static_assert(sizeof(DebugEventInfo) >= 0x50, "Incorrect DebugEventInfo definition!");