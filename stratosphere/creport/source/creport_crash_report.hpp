#pragma once

#include <switch.h>

class CrashReport {
    private:
        Handle debug_handle;
        bool has_extra_info;
        Result result;
        
    public:
        CrashReport() : debug_handle(INVALID_HANDLE), result(0x4A2) { }
        
        void BuildReport(u64 pid, bool has_extra_info);
        
        Result GetResult() {
            return this->result;
        }
        
        bool OpenProcess(u64 pid) {
            return R_SUCCEEDED(svcDebugActiveProcess(&debug_handle, pid));
        }
        
        void Close() {
            if (debug_handle != INVALID_HANDLE) {
                svcCloseHandle(debug_handle);
                debug_handle = INVALID_HANDLE;
            }
        }
};