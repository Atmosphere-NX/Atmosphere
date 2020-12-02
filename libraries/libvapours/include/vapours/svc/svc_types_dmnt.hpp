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
#include <vapours/svc/svc_types_common.hpp>

namespace ams::svc {

    namespace lp64 {

        struct DebugInfoCreateProcess {
            u64 program_id;
            u64 process_id;
            char name[0xC];
            u32 flags;
            u64 user_exception_context_address; /* 5.0.0+ */
        };

        struct DebugInfoCreateThread {
            u64 thread_id;
            u64 tls_address;
            /* Removed in 11.0.0 u64 entrypoint; */
        };

        struct DebugInfoExitProcess {
            ProcessExitReason reason;
        };

        struct DebugInfoExitThread {
            ThreadExitReason reason;
        };

        struct DebugInfoUndefinedInstructionException {
            u32 insn;
        };

        struct DebugInfoDataAbortException {
            u64 address;
        };

        struct DebugInfoAlignmentFaultException {
            u64 address;
        };

        struct DebugInfoBreakPointException {
            BreakPointType type;
            u64 address;
        };

        struct DebugInfoUserBreakException {
            BreakReason break_reason;
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
            DebugInfoAlignmentFaultException alignment_fault;
            DebugInfoBreakPointException break_point;
            DebugInfoUserBreakException user_break;
            DebugInfoDebuggerBreakException debugger_break;
            DebugInfoUndefinedSystemCallException undefined_system_call;
            u64 raw;
        };

        struct DebugInfoException {
            DebugException type;
            u64 address;
            DebugInfoSpecificException specific;
        };

        union DebugInfo {
            DebugInfoCreateProcess create_process;
            DebugInfoCreateThread create_thread;
            DebugInfoExitProcess exit_process;
            DebugInfoExitThread exit_thread;
            DebugInfoException exception;
        };

        struct DebugEventInfo {
            DebugEvent type;
            u32 flags;
            u64 thread_id;
            DebugInfo info;
        };
        static_assert(sizeof(DebugEventInfo) >= 0x40);

    }

    namespace ilp32 {

        struct DebugInfoCreateProcess {
            u64 program_id;
            u64 process_id;
            char name[0xC];
            u32 flags;
            u32 user_exception_context_address; /* 5.0.0+ */
        };

        struct DebugInfoCreateThread {
            u64 thread_id;
            u32 tls_address;
            u32 entrypoint;
        };

        struct DebugInfoExitProcess {
            ProcessExitReason reason;
        };

        struct DebugInfoExitThread {
            ThreadExitReason reason;
        };

        struct DebugInfoUndefinedInstructionException {
            u32 insn;
        };

        struct DebugInfoDataAbortException {
            u32 address;
        };

        struct DebugInfoAlignmentFaultException {
            u32 address;
        };

        struct DebugInfoBreakPointException {
            BreakPointType type;
            u32 address;
        };

        struct DebugInfoUserBreakException {
            BreakReason break_reason;
            u32 address;
            u32 size;
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
            DebugInfoAlignmentFaultException alignment_fault;
            DebugInfoBreakPointException break_point;
            DebugInfoUserBreakException user_break;
            DebugInfoDebuggerBreakException debugger_break;
            DebugInfoUndefinedSystemCallException undefined_system_call;
            u64 raw;
        };

        struct DebugInfoException {
            DebugException type;
            u32 address;
            DebugInfoSpecificException specific;
        };

        union DebugInfo {
            DebugInfoCreateProcess create_process;
            DebugInfoCreateThread create_thread;
            DebugInfoExitProcess exit_process;
            DebugInfoExitThread exit_thread;
            DebugInfoException exception;
        };

        struct DebugEventInfo {
            DebugEvent type;
            u32 flags;
            u64 thread_id;
            DebugInfo info;
        };

    }

}
