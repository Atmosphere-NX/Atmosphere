#include <switch.h>
#include "creport_crash_report.hpp"
#include "creport_debug_types.hpp"

void CrashReport::SaveReport() {
    /* TODO: Save the report to the SD card. */
}

void CrashReport::BuildReport(u64 pid, bool has_extra_info) {
    this->has_extra_info = has_extra_info;
    if (OpenProcess(pid)) {
        ProcessExceptions();
        
        /* TODO: More stuff here (sub_7100002260)... */
        Close();
    }
}

void CrashReport::ProcessExceptions() {
    if (!IsOpen()) {
        return;
    }
    
    DebugEventInfo d;
    while (R_SUCCEEDED(svcGetDebugEvent((u8 *)&d, this->debug_handle))) {
        switch (d.type) {
            case DebugEventType::AttachProcess:
                HandleAttachProcess(d);
                break;
            case DebugEventType::Exception:
                HandleException(d);
                break;
            case DebugEventType::AttachThread:
            case DebugEventType::ExitProcess:
            case DebugEventType::ExitThread:
            default:
                break;
        }
    }
}

void CrashReport::HandleAttachProcess(DebugEventInfo &d) {
    this->process_info = d.info.attach_process;
    if (kernelAbove500() && IsApplication()) {
        /* Parse out user data. */
        MemoryInfo mi;
        u32 pi;
        u64 address = this->process_info.user_exception_context_address;
        u64 userdata_address = 0;
        u64 userdata_size = 0;
        
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, this->debug_handle, address))) {
            return;
        }
        
        /* Must be read or read-write */
        if ((mi.perm | Perm_W) != Perm_Rw) {
            return;
        }
        
        /* Must have space for both userdata address and userdata size. */
        if (address < mi.addr || mi.addr + mi.size < address + 2 * sizeof(u64)) {
            return;
        }
        
        /* Read userdata address. */
        if (R_FAILED(svcReadDebugProcessMemory(&userdata_address, this->debug_handle, address, sizeof(userdata_address)))) {
            return;
        }
        
        /* Validate userdata address. */
        if (userdata_address == 0 || userdata_address & 0xFFF) {
            return;
        }
        
        /* Read userdata size. */
        if (R_FAILED(svcReadDebugProcessMemory(&userdata_size, this->debug_handle, address + sizeof(userdata_address), sizeof(userdata_size)))) {
            return;
        }
        
        /* Cap userdata size. */
        if (userdata_size > 0x1000) {
            userdata_size = 0x1000;
        }
        
        /* Assign. */
        this->userdata_5x_address = userdata_address;
        this->userdata_5x_size = userdata_size;
    }
}

void CrashReport::HandleException(DebugEventInfo &d) {
    switch (d.info.exception.type) {
        case DebugExceptionType::UndefinedInstruction:
        case DebugExceptionType::InstructionAbort:
        case DebugExceptionType::DataAbort:
        case DebugExceptionType::AlignmentFault:
        case DebugExceptionType::UserBreak:
        case DebugExceptionType::BadSvc:
        case DebugExceptionType::UnknownNine:
            /* TODO: Handle these exceptions...creport seems to discard all but the latest exception? */
            break;
        case DebugExceptionType::DebuggerAttached:
        case DebugExceptionType::BreakPoint:
        case DebugExceptionType::DebuggerBreak:
        default:
            break;
    }
}