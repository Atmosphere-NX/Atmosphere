#pragma once

#include <switch.h>

#include "creport_debug_types.hpp"
#include "creport_thread_info.hpp"

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
        Handle debug_handle;
        bool has_extra_info;
        Result result;
        
        /* Attach Process Info. */ 
        AttachProcessInfo process_info;
        u64 userdata_5x_address;
        u64 userdata_5x_size;
        
        /* Exception Info. */
        ExceptionInfo exception_info;
        ThreadInfo crashed_thread_info;
        
    public:
        CrashReport() : debug_handle(INVALID_HANDLE), result((Result)CrashReportResult::IncompleteReport), process_info({}), exception_info({}) { }
        
        void BuildReport(u64 pid, bool has_extra_info);
        void SaveReport();
        void ProcessExceptions();
        
        bool IsAddressReadable(u64 address, u64 size, MemoryInfo *mi = NULL);
        
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
        void HandleAttachProcess(DebugEventInfo &d);
        void HandleException(DebugEventInfo &d);
};