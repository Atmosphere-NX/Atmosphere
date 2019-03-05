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
#include <cstdio>

#include "creport_debug_types.hpp"
#include "creport_thread_info.hpp"
#include "creport_code_info.hpp"

enum class CrashReportResult : Result {
    UndefinedInstruction = 0x00A8,
    InstructionAbort = 0x02A8,
    DataAbort = 0x04A8,
    AlignmentFault = 0x06A8,
    DebuggerAttached = 0x08A8,
    BreakPoint = 0x0AA8,
    UserBreak = 0x0CA8,
    DebuggerBreak = 0x0EA8,
    BadSvc = 0x10A8,
    UnknownNine = 0x12A8,
    IncompleteReport = 0xC6A8,
};

class CrashReport {
    private:
        Handle debug_handle = INVALID_HANDLE;
        bool has_extra_info;
        Result result = static_cast<Result>(CrashReportResult::IncompleteReport);
        
        /* Attach Process Info. */ 
        AttachProcessInfo process_info{};
        u64 dying_message_address = 0;
        u64 dying_message_size = 0;
        u8 dying_message[0x1000]{};
        
        static_assert(sizeof(dying_message) == 0x1000, "Incorrect definition for dying message!");
        
        /* Exception Info. */
        ExceptionInfo exception_info{};
        ThreadInfo crashed_thread_info;
        
        /* Extra Info. */
        CodeList code_list;
        ThreadList thread_list;
        
    public:
        void BuildReport(u64 pid, bool has_extra_info);
        FatalContext *GetFatalContext();
        void SaveReport();
        
        bool IsAddressReadable(u64 address, u64 size, MemoryInfo *mi = NULL);
        
        static void Memdump(FILE *f, const char *prefix, const void *data, size_t size);
        
        Result GetResult() {
            return this->result;
        }
        
        bool WasSuccessful() {
            return this->result != (Result)CrashReportResult::IncompleteReport;
        }
        
        bool OpenProcess(u64 pid) {
            return R_SUCCEEDED(svcDebugActiveProcess(&debug_handle, pid));
        }
        
        bool IsOpen() {
            return this->debug_handle != INVALID_HANDLE;
        }
        
        void Close() {
            if (IsOpen()) {
                svcCloseHandle(debug_handle);
                debug_handle = INVALID_HANDLE;
            }
        }
        
        bool IsApplication() {
            return (process_info.flags & 0x40) != 0;
        }
        
        bool Is64Bit() {
            return (process_info.flags & 0x01) != 0;
        }
        
        bool IsUserBreak() {
            return this->exception_info.type == DebugExceptionType::UserBreak;
        }
    private:
        void ProcessExceptions();
        void ProcessDyingMessage();
        void HandleAttachProcess(DebugEventInfo &d);
        void HandleException(DebugEventInfo &d);
        
        void SaveToFile(FILE *f);
        
        void EnsureReportDirectories();
        bool GetCurrentTime(u64 *out);
};
