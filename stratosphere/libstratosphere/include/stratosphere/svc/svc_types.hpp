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
#include <cstring>
#include <switch.h>
#include "../defines.hpp"
#include "../results.hpp"

namespace sts::svc {

    /* Debug event types. */
    enum class DebugEventType : u32 {
        AttachProcess = 0,
        AttachThread  = 1,
        ExitProcess   = 2,
        ExitThread    = 3,
        Exception     = 4,
    };

    struct DebugInfoAttachProcess {
        u64 title_id;
        u64 process_id;
        char name[0xC];
        u32 flags;
        u64 user_exception_context_address; /* 5.0.0+ */
    };

    struct DebugInfoAttachThread {
        u64 thread_id;
        u64 tls_address;
        u64 entrypoint;
    };

    enum class ExitProcessReason : u32 {
        ExitProcess      = 0,
        TerminateProcess = 1,
        Exception        = 2,
    };

    struct DebugInfoExitProcess {
        ExitProcessReason reason;
    };

    enum class ExitThreadReason : u32 {
        ExitThread       = 0,
        TerminateThread  = 1,
        ExitProcess      = 2,
        TerminateProcess = 3,
    };

    struct DebugInfoExitThread {
        ExitThreadReason reason;
    };

    enum class DebugExceptionType : u32 {
        UndefinedInstruction = 0,
        InstructionAbort     = 1,
        DataAbort            = 2,
        AlignmentFault       = 3,
        DebuggerAttached     = 4,
        BreakPoint           = 5,
        UserBreak            = 6,
        DebuggerBreak        = 7,
        UndefinedSystemCall  = 8,
        SystemMemoryError    = 9,
    };

    struct DebugInfoUndefinedInstructionException {
        u32 insn;
    };

    struct DebugInfoDataAbortException {
        u64 address;
    };

    struct DebugInfoAligntmentFaultException {
        u64 address;
    };

    enum class BreakPointType : u32 {
        BreakPoint = 0,
        WatchPoint = 1,
    };

    struct DebugInfoBreakPointException {
        BreakPointType type;
        u64 address;
    };

    struct DebugInfoUserBreakException {
        u32 break_reason; /* TODO: enum? */
        u64 address;
        u64 size;
    };

    struct DebugInfoDebuggerBreakException {
        u64 active_thread_ids[4];
    };

    struct DebugInfoUndefinedSystemCallException {
        u32 id;
    };

    union DebugInfoSpecificException {
        DebugInfoUndefinedInstructionException undefined_instruction;
        DebugInfoDataAbortException data_abort;
        DebugInfoAligntmentFaultException alignment_fault;
        DebugInfoBreakPointException break_point;
        DebugInfoUserBreakException user_break;
        DebugInfoDebuggerBreakException debugger_break;
        DebugInfoUndefinedSystemCallException undefined_system_call;
        u64 raw;
    };

    struct DebugInfoException {
        DebugExceptionType type;
        u64 address;
        DebugInfoSpecificException specific;
    };

    union DebugInfo {
        DebugInfoAttachProcess attach_process;
        DebugInfoAttachThread attach_thread;
        DebugInfoExitProcess exit_process;
        DebugInfoExitThread exit_thread;
        DebugInfoException exception;
    };

    struct DebugEventInfo {
        DebugEventType type;
        u32 flags;
        u64 thread_id;
        DebugInfo info;
    };
    static_assert(sizeof(DebugEventInfo) >= 0x40, "DebugEventInfo definition!");

    /* Thread State, for svcGetDebugThreadParam. */
    enum class ThreadState : u32 {
        Waiting      = 0,
        Running      = 1,
        Terminated   = 4,
        Initializing = 5,
    };

    enum ThreadContextFlag : u32 {
        ThreadContextFlag_General    = (1 << 0),
        ThreadContextFlag_Control    = (1 << 1),
        ThreadContextFlag_Fpu        = (1 << 2),
        ThreadContextFlag_FpuControl = (1 << 3),

        ThreadContextFlag_All = (ThreadContextFlag_General | ThreadContextFlag_Control | ThreadContextFlag_Fpu | ThreadContextFlag_FpuControl),
    };

    /* Flags for svcCreateProcess. */
    enum CreateProcessFlag : u32 {
        /* Is 64 bit? */
        CreateProcessFlag_Is64Bit       = (1 << 0),

        /* What kind of address space? */
        CreateProcessFlag_AddressSpaceShift             = 1,
        CreateProcessFlag_AddressSpaceMask              = (7 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace32Bit             = (0 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace64BitDeprecated   = (1 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace32BitWithoutAlias = (2 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace64Bit             = (3 << CreateProcessFlag_AddressSpaceShift),

        /* Should JIT debug be done on crash? */
        CreateProcessFlag_EnableDebug   = (1 << 4),

        /* Should ASLR be enabled for the process? */
        CreateProcessFlag_EnableAslr    = (1 << 5),

        /* Is the process an application? */
        CreateProcessFlag_IsApplication = (1 << 6),

        /* 4.x deprecated: Should use secure memory? */
        CreateProcessFlag_DeprecatedUseSecureMemory = (1 << 7),

        /* 5.x+ Pool partition type. */
        CreateProcessFlag_PoolPartitionShift            = 7,
        CreateProcessFlag_PoolPartitionMask             = (0xF << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionApplication      = (0 << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionApplet           = (1 << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionSystem           = (2 << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionSystemNonSecure  = (3 << CreateProcessFlag_PoolPartitionShift),

        /* 7.x+ Should memory allocation be optimized? This requires IsApplication. */
        CreateProcessFlag_OptimizeMemoryAllocation = (1 << 11),
    };

}
