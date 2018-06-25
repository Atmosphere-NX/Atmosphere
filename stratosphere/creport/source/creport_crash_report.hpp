#pragma once

#include <switch.h>

#include "creport_debug_types.hpp"

class CrashReport {
    private:
        Handle debug_handle;
        bool has_extra_info;
        Result result;
        
        /* Attach Process Info. */ 
        AttachProcessInfo process_info;
        u64 userdata_5x_address;
        u64 userdata_5x_size;
        
    public:
        CrashReport() : debug_handle(INVALID_HANDLE), result(0xC6A8), process_info({0}) { }
        
        void BuildReport(u64 pid, bool has_extra_info);
        void ProcessExceptions();
        
        Result GetResult() {
            return this->result;
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
    private:
        void HandleAttachProcess(DebugEventInfo &d);
        void HandleException(DebugEventInfo &d);
};