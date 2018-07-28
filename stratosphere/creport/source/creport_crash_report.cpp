#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <switch.h>

#include "creport_crash_report.hpp"
#include "creport_debug_types.hpp"

void CrashReport::BuildReport(u64 pid, bool has_extra_info) {
    this->has_extra_info = has_extra_info;
    if (OpenProcess(pid)) {
        ProcessExceptions();
        if (kernelAbove500()) {
            this->code_list.ReadCodeRegionsFromProcess(this->debug_handle, this->crashed_thread_info.GetPC(), this->crashed_thread_info.GetLR());
            this->thread_list.ReadThreadsFromProcess(this->debug_handle, Is64Bit());
        }
        this->thread_list.SetCodeList(&this->code_list);
        
        if (IsApplication()) {
            ProcessDyingMessage();
        }
        
        /* Real creport builds the report here. We do it later. */
        
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
        u64 address = this->process_info.user_exception_context_address;
        u64 userdata_address = 0;
        u64 userdata_size = 0;
        
        if (!IsAddressReadable(address, sizeof(userdata_address) + sizeof(userdata_size))) {
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
        if (userdata_size > sizeof(this->dying_message)) {
            userdata_size = sizeof(this->dying_message);
        }
        
        /* Assign. */
        this->dying_message_address = userdata_address;
        this->dying_message_size = userdata_size;
    }
}

void CrashReport::HandleException(DebugEventInfo &d) {
    switch (d.info.exception.type) {
        case DebugExceptionType::UndefinedInstruction:
            this->result = (Result)CrashReportResult::UndefinedInstruction;
            break;
        case DebugExceptionType::InstructionAbort:
            this->result = (Result)CrashReportResult::InstructionAbort;
            d.info.exception.specific.raw = 0;
            break;
        case DebugExceptionType::DataAbort:
            this->result = (Result)CrashReportResult::DataAbort;
            break;
        case DebugExceptionType::AlignmentFault:
            this->result = (Result)CrashReportResult::AlignmentFault;
            break;
        case DebugExceptionType::UserBreak:
            this->result = (Result)CrashReportResult::UserBreak;
            /* Try to parse out the user break result. */
            if (kernelAbove500() && IsAddressReadable(d.info.exception.specific.user_break.address, sizeof(this->result))) {
                svcReadDebugProcessMemory(&this->result, this->debug_handle, d.info.exception.specific.user_break.address, sizeof(this->result));
            }
            break;
        case DebugExceptionType::BadSvc:
            this->result = (Result)CrashReportResult::BadSvc;
            break;
        case DebugExceptionType::UnknownNine:
            this->result = (Result)CrashReportResult::UnknownNine;
            d.info.exception.specific.raw = 0;
            break;
        case DebugExceptionType::DebuggerAttached:
        case DebugExceptionType::BreakPoint:
        case DebugExceptionType::DebuggerBreak:
        default:
            return;
    }
    this->exception_info = d.info.exception;
    /* Parse crashing thread info. */
    this->crashed_thread_info.ReadFromProcess(this->debug_handle, d.thread_id, Is64Bit());
}

void CrashReport::ProcessDyingMessage() {
    /* Dying message is only stored starting in 5.0.0. */
    if (!kernelAbove500()) {
        return;
    }
    
    /* Validate the message address/size. */
    if (this->dying_message_address == 0 || this->dying_message_address & 0xFFF) {
        return;
    }
    if (this->dying_message_size > sizeof(this->dying_message)) {
        return;
    }
    
    /* Validate that the report isn't garbage. */
    if (!IsOpen() || !WasSuccessful()) {
        return;
    }
    
    if (!IsAddressReadable(this->dying_message_address, this->dying_message_size)) {
        return;
    }
    
    svcReadDebugProcessMemory(this->dying_message, this->debug_handle, this->dying_message_address, this->dying_message_size);
}

bool CrashReport::IsAddressReadable(u64 address, u64 size, MemoryInfo *o_mi) {
    MemoryInfo mi;
    u32 pi;
    
    if (o_mi == NULL) {
        o_mi = &mi;
    }
    
    if (R_FAILED(svcQueryDebugProcessMemory(o_mi, &pi, this->debug_handle, address))) {
        return false;
    }
    
    /* Must be read or read-write */
    if ((o_mi->perm | Perm_W) != Perm_Rw) {
        return false;
    }
    
    /* Must have space for both userdata address and userdata size. */
    if (address < o_mi->addr || o_mi->addr + o_mi->size < address + size) {
        return false;
    }

    return true;
}

bool CrashReport::GetCurrentTime(u64 *out) {
    *out = 0;
    
    /* Verify that pcv isn't dead. */
    {
        Handle dummy;
        if (R_SUCCEEDED(smRegisterService(&dummy, "time:s", false, 0x20))) {
            svcCloseHandle(dummy);
            return false;
        }
    }
    
    /* Try to get the current time. */
    bool success = false;
    if (R_SUCCEEDED(timeInitialize())) {
        if (R_SUCCEEDED(timeGetCurrentTime(TimeType_LocalSystemClock, out))) {
            success = true;
        }
        timeExit();
    }
    return success;
}

void CrashReport::EnsureReportDirectories() {
    char path[FS_MAX_PATH];  
    strcpy(path, "sdmc:/atmosphere");
    mkdir(path, S_IRWXU);
    strcat(path, "/crash reports");
    mkdir(path, S_IRWXU);
    strcat(path, "/dumps");
    mkdir(path, S_IRWXU);
}

void CrashReport::SaveReport() {
    /* TODO: Save the report to the SD card. */
    char report_path[FS_MAX_PATH];
    
    /* Ensure path exists. */
    EnsureReportDirectories();
    
    /* Get a timestamp. */
    u64 timestamp;
    if (!GetCurrentTime(&timestamp)) {
        timestamp = svcGetSystemTick();
    }
    
    /* Open report file. */
    snprintf(report_path, sizeof(report_path) - 1, "sdmc:/atmosphere/crash reports/%011lu_%016lx.log", timestamp, process_info.title_id);
    FILE *f_report = fopen(report_path, "w");
    if (f_report == NULL) {
        return;
    }
    this->SaveToFile(f_report);
    fclose(f_report);
    
    /* Dump threads. */
    snprintf(report_path, sizeof(report_path) - 1, "sdmc:/atmosphere/crash reports/dumps/%011lu_%016lx_thread_info.bin", timestamp, process_info.title_id);
    f_report = fopen(report_path, "wb");
    this->thread_list.DumpBinary(f_report, this->crashed_thread_info.GetId());
    fclose(f_report);
}

void CrashReport::SaveToFile(FILE *f_report) {
    char buf[0x10] = {0};
    fprintf(f_report, "Atmosphère Crash Report (v1.1):\n");
    fprintf(f_report, "Result:                          0x%X (2%03d-%04d)\n\n", this->result, R_MODULE(this->result), R_DESCRIPTION(this->result));
    
    /* Process Info. */
    memcpy(buf, this->process_info.name, sizeof(this->process_info.name));
    fprintf(f_report, "Process Info:\n");
    fprintf(f_report, "    Process Name:                %s\n", buf);
    fprintf(f_report, "    Title ID:                    %016lx\n", this->process_info.title_id);
    fprintf(f_report, "    Process ID:                  %016lx\n", this->process_info.process_id);
    fprintf(f_report, "    Process Flags:               %08x\n", this->process_info.flags);
    if (kernelAbove500()) {
        fprintf(f_report, "    User Exception Address:      %s\n", this->code_list.GetFormattedAddressString(this->process_info.user_exception_context_address));
    }
    
    fprintf(f_report, "Exception Info:\n");
    fprintf(f_report, "    Type:                        %s\n", GetDebugExceptionTypeStr(this->exception_info.type));
    fprintf(f_report, "    Address:                     %s\n", this->code_list.GetFormattedAddressString(this->exception_info.address));
    switch (this->exception_info.type) {
        case DebugExceptionType::UndefinedInstruction:
            fprintf(f_report, "    Opcode:                      %08x\n", this->exception_info.specific.undefined_instruction.insn);
            break;
        case DebugExceptionType::DataAbort:
        case DebugExceptionType::AlignmentFault:
            if (this->exception_info.specific.raw != this->exception_info.address) { 
                fprintf(f_report, "    Fault Address:               %s\n", this->code_list.GetFormattedAddressString(this->exception_info.specific.raw));
            }
            break;
        case DebugExceptionType::BadSvc:
            fprintf(f_report, "    Svc Id:                      0x%02x\n", this->exception_info.specific.bad_svc.id);
            break;
        default:
            break;
    }
    
    fprintf(f_report, "Crashed Thread Info:\n");
    this->crashed_thread_info.SaveToFile(f_report);
    
    if (kernelAbove500()) {
        if (this->dying_message_size) {
            fprintf(f_report, "Dying Message Info:\n");
            fprintf(f_report, "    Address:                     0x%s\n", this->code_list.GetFormattedAddressString(this->dying_message_address));
            fprintf(f_report, "    Size:                        0x%016lx\n", this->dying_message_size);
            CrashReport::Memdump(f_report, "    Dying Message:              ", this->dying_message, this->dying_message_size);
        }
        fprintf(f_report, "Code Region Info:\n");
        this->code_list.SaveToFile(f_report);
        fprintf(f_report, "\nThread Report:\n");
        this->thread_list.SaveToFile(f_report);
    }
}

/* Lifted from hactool. */
void CrashReport::Memdump(FILE *f, const char *prefix, const void *data, size_t size) {
    uint8_t *p = (uint8_t *)data;

    unsigned int prefix_len = strlen(prefix);
    size_t offset = 0;
    int first = 1;

    while (size) {
        unsigned int max = 32;

        if (max > size) {
            max = size;
        }

        if (first) {
            fprintf(f, "%s", prefix);
            first = 0;
        } else {
            fprintf(f, "%*s", prefix_len, "");
        }

        for (unsigned int i = 0; i < max; i++) {
            fprintf(f, "%02X", p[offset++]);
        }

        fprintf(f, "\n");

        size -= max;
    }
}
