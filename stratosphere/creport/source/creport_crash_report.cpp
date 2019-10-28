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
#include <sys/stat.h>
#include <sys/types.h>
#include "creport_crash_report.hpp"
#include "creport_utils.hpp"

namespace ams::creport {

    namespace {

        /* Convenience definitions. */
        constexpr size_t DyingMessageAddressOffset = 0x1C0;

        /* Helper functions. */
        bool TryGetCurrentTimestamp(u64 *out) {
            /* Clear output. */
            *out = 0;

            /* Check if we have time service. */
            {
                bool has_time_service = false;
                if (R_FAILED(sm::HasService(&has_time_service, sm::ServiceName::Encode("time:s"))) || !has_time_service) {
                    return false;
                }
            }

            /* Try to get the current time. */
            {
                sm::ScopedServiceHolder<timeInitialize, timeExit> time_holder;
                return time_holder && R_SUCCEEDED(timeGetCurrentTime(TimeType_LocalSystemClock, out));
            }
        }

        void TryCreateReportDirectories() {
            mkdir("sdmc:/atmosphere", S_IRWXU);
            mkdir("sdmc:/atmosphere/crash_reports", S_IRWXU);
            mkdir("sdmc:/atmosphere/crash_reports/dumps", S_IRWXU);
            mkdir("sdmc:/atmosphere/fatal_reports", S_IRWXU);
            mkdir("sdmc:/atmosphere/fatal_reports/dumps", S_IRWXU);
        }

        constexpr const char *GetDebugExceptionTypeString(const svc::DebugExceptionType type) {
            switch (type) {
                case svc::DebugExceptionType::UndefinedInstruction:
                    return "Undefined Instruction";
                case svc::DebugExceptionType::InstructionAbort:
                    return "Instruction Abort";
                case svc::DebugExceptionType::DataAbort:
                    return "Data Abort";
                case svc::DebugExceptionType::AlignmentFault:
                    return "Alignment Fault";
                case svc::DebugExceptionType::DebuggerAttached:
                    return "Debugger Attached";
                case svc::DebugExceptionType::BreakPoint:
                    return "Break Point";
                case svc::DebugExceptionType::UserBreak:
                    return "User Break";
                case svc::DebugExceptionType::DebuggerBreak:
                    return "Debugger Break";
                case svc::DebugExceptionType::UndefinedSystemCall:
                    return "Undefined System Call";
                case svc::DebugExceptionType::SystemMemoryError:
                    return "System Memory Error";
                default:
                    return "Unknown";
            }
        }

    }

    void CrashReport::BuildReport(os::ProcessId process_id, bool has_extra_info) {
        this->has_extra_info = has_extra_info;

        if (this->OpenProcess(process_id)) {
            ON_SCOPE_EXIT { this->Close(); };

            /* Parse info from the crashed process. */
            this->ProcessExceptions();
            this->module_list.FindModulesFromThreadInfo(this->debug_handle, this->crashed_thread);
            this->thread_list.ReadFromProcess(this->debug_handle, this->thread_tls_map, this->Is64Bit());

            /* Associate module list to threads. */
            this->crashed_thread.SetModuleList(&this->module_list);
            this->thread_list.SetModuleList(&this->module_list);

            /* Process dying message for applications. */
            if (this->IsApplication()) {
                this->ProcessDyingMessage();
            }

            /* Nintendo's creport finds extra modules by looking at all threads if application, */
            /* but there's no reason for us not to always go looking. */
            for (size_t i = 0; i < this->thread_list.GetThreadCount(); i++) {
                this->module_list.FindModulesFromThreadInfo(this->debug_handle, this->thread_list.GetThreadInfo(i));
            }

            /* Nintendo's creport saves the report to erpt here, but we'll save to SD card later. */
        }
    }

    void CrashReport::GetFatalContext(::FatalCpuContext *_out) const {
        static_assert(sizeof(*_out) == sizeof(ams::fatal::CpuContext));
        ams::fatal::CpuContext *out = reinterpret_cast<ams::fatal::CpuContext *>(_out);
        std::memset(out, 0, sizeof(*out));

        /* TODO: Support generating 32-bit fatal contexts? */
        out->architecture = fatal::CpuContext::Architecture_Aarch64;
        out->type = static_cast<u32>(this->exception_info.type);

        for (size_t i = 0; i < fatal::aarch64::RegisterName_FP; i++) {
            out->aarch64_ctx.SetRegisterValue(static_cast<fatal::aarch64::RegisterName>(i), this->crashed_thread.GetGeneralPurposeRegister(i));
        }
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_FP, this->crashed_thread.GetFP());
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_LR, this->crashed_thread.GetLR());
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_SP, this->crashed_thread.GetSP());
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_PC, this->crashed_thread.GetPC());

        out->aarch64_ctx.stack_trace_size = this->crashed_thread.GetStackTraceSize();
        for (size_t i = 0; i < out->aarch64_ctx.stack_trace_size; i++) {
            out->aarch64_ctx.stack_trace[i] = this->crashed_thread.GetStackTrace(i);
        }

        if (this->module_list.GetModuleCount()) {
            out->aarch64_ctx.SetBaseAddress(this->module_list.GetModuleStartAddress(0));
        }

        /* For ams fatal, which doesn't use afsr0, pass program_id instead. */
        out->aarch64_ctx.SetProgramIdForAtmosphere(ncm::ProgramId{this->process_info.program_id});
    }

    void CrashReport::ProcessExceptions() {
        /* Loop all debug events. */
        svc::DebugEventInfo d;
        while (R_SUCCEEDED(svcGetDebugEvent(reinterpret_cast<u8 *>(&d), this->debug_handle))) {
            switch (d.type) {
                case svc::DebugEventType::AttachProcess:
                    this->HandleDebugEventInfoAttachProcess(d);
                    break;
                case svc::DebugEventType::AttachThread:
                    this->HandleDebugEventInfoAttachThread(d);
                    break;
                case svc::DebugEventType::Exception:
                    this->HandleDebugEventInfoException(d);
                    break;
                case svc::DebugEventType::ExitProcess:
                case svc::DebugEventType::ExitThread:
                    break;
            }
        }

        /* Parse crashed thread info. */
        this->crashed_thread.ReadFromProcess(this->debug_handle, this->thread_tls_map, this->crashed_thread_id, this->Is64Bit());
    }

    void CrashReport::HandleDebugEventInfoAttachProcess(const svc::DebugEventInfo &d) {
        this->process_info = d.info.attach_process;

        /* On 5.0.0+, we want to parse out a dying message from application crashes. */
        if (hos::GetVersion() < hos::Version_500 || !IsApplication()) {
            return;
        }

        /* Parse out user data. */
        const u64 address = this->process_info.user_exception_context_address + DyingMessageAddressOffset;
        u64 userdata_address = 0;
        u64 userdata_size = 0;

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
        userdata_size = std::min(size_t(userdata_size), sizeof(this->dying_message));

        this->dying_message_address = userdata_address;
        this->dying_message_size = userdata_size;
    }

    void CrashReport::HandleDebugEventInfoAttachThread(const svc::DebugEventInfo &d) {
        /* Save info on the thread's TLS address for later. */
        this->thread_tls_map[d.info.attach_thread.thread_id] = d.info.attach_thread.tls_address;
    }

    void CrashReport::HandleDebugEventInfoException(const svc::DebugEventInfo &d) {
        switch (d.info.exception.type) {
            case svc::DebugExceptionType::UndefinedInstruction:
                this->result = ResultUndefinedInstruction();
                break;
            case svc::DebugExceptionType::InstructionAbort:
                this->result = ResultInstructionAbort();
                break;
            case svc::DebugExceptionType::DataAbort:
                this->result = ResultDataAbort();
                break;
            case svc::DebugExceptionType::AlignmentFault:
                this->result = ResultAlignmentFault();
                break;
            case svc::DebugExceptionType::UserBreak:
                this->result = ResultUserBreak();
                /* Try to parse out the user break result. */
                if (hos::GetVersion() >= hos::Version_500) {
                    svcReadDebugProcessMemory(&this->result, this->debug_handle, d.info.exception.specific.user_break.address, sizeof(this->result));
                }
                break;
            case svc::DebugExceptionType::UndefinedSystemCall:
                this->result = ResultUndefinedSystemCall();
                break;
            case svc::DebugExceptionType::SystemMemoryError:
                this->result = ResultSystemMemoryError();
                break;
            case svc::DebugExceptionType::DebuggerAttached:
            case svc::DebugExceptionType::BreakPoint:
            case svc::DebugExceptionType::DebuggerBreak:
                return;
        }

        /* Save exception info. */
        this->exception_info = d.info.exception;
        this->crashed_thread_id = d.thread_id;
    }

    void CrashReport::ProcessDyingMessage() {
        /* Dying message is only stored starting in 5.0.0. */
        if (hos::GetVersion() < hos::Version_500) {
            return;
        }

        /* Validate address/size. */
        if (this->dying_message_address == 0 || this->dying_message_address & 0xFFF) {
            return;
        }
        if (this->dying_message_size > sizeof(this->dying_message)) {
            return;
        }

        /* Validate that the current report isn't garbage. */
        if (!IsOpen() || !IsComplete()) {
            return;
        }

        /* Read the dying message. */
        svcReadDebugProcessMemory(this->dying_message, this->debug_handle, this->dying_message_address, this->dying_message_size);
    }

    void CrashReport::SaveReport() {
        /* Try to ensure path exists. */
        TryCreateReportDirectories();

        /* Get a timestamp. */
        u64 timestamp;
        if (!TryGetCurrentTimestamp(&timestamp)) {
            timestamp = svcGetSystemTick();
        }

        /* Save files. */
        {
            char file_path[FS_MAX_PATH];

            /* Save crash report. */
            std::snprintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/%011lu_%016lx.log", timestamp, this->process_info.program_id);
            FILE *fp = fopen(file_path, "w");
            if (fp != nullptr) {
                this->SaveToFile(fp);
                fclose(fp);
                fp = nullptr;
            }

            /* Dump threads. */
            std::snprintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/dumps/%011lu_%016lx_thread_info.bin", timestamp, this->process_info.program_id);
            fp = fopen(file_path, "wb");
            if (fp != nullptr) {
                this->thread_list.DumpBinary(fp, this->crashed_thread.GetThreadId());
                fclose(fp);
                fp = nullptr;
            }
        }
    }

    void CrashReport::SaveToFile(FILE *f_report) {
        fprintf(f_report, "Atmosphère Crash Report (v1.4):\n");
        fprintf(f_report, "Result:                          0x%X (2%03d-%04d)\n\n", this->result.GetValue(), this->result.GetModule(), this->result.GetDescription());

        /* Process Info. */
        char name_buf[0x10] = {};
        static_assert(sizeof(name_buf) >= sizeof(this->process_info.name), "buffer overflow!");
        std::memcpy(name_buf, this->process_info.name, sizeof(this->process_info.name));
        fprintf(f_report, "Process Info:\n");
        fprintf(f_report, "    Process Name:                %s\n", name_buf);
        fprintf(f_report, "    Program ID:                  %016lx\n", this->process_info.program_id);
        fprintf(f_report, "    Process ID:                  %016lx\n", this->process_info.process_id);
        fprintf(f_report, "    Process Flags:               %08x\n", this->process_info.flags);
        if (hos::GetVersion() >= hos::Version_500) {
            fprintf(f_report, "    User Exception Address:      %s\n", this->module_list.GetFormattedAddressString(this->process_info.user_exception_context_address));
        }

        /* Exception Info. */
        fprintf(f_report, "Exception Info:\n");
        fprintf(f_report, "    Type:                        %s\n", GetDebugExceptionTypeString(this->exception_info.type));
        fprintf(f_report, "    Address:                     %s\n", this->module_list.GetFormattedAddressString(this->exception_info.address));
        switch (this->exception_info.type) {
            case svc::DebugExceptionType::UndefinedInstruction:
                fprintf(f_report, "    Opcode:                      %08x\n", this->exception_info.specific.undefined_instruction.insn);
                break;
            case svc::DebugExceptionType::DataAbort:
            case svc::DebugExceptionType::AlignmentFault:
                if (this->exception_info.specific.raw != this->exception_info.address) {
                    fprintf(f_report, "    Fault Address:               %s\n", this->module_list.GetFormattedAddressString(this->exception_info.specific.raw));
                }
                break;
            case svc::DebugExceptionType::UndefinedSystemCall:
                fprintf(f_report, "    Svc Id:                      0x%02x\n", this->exception_info.specific.undefined_system_call.id);
                break;
            case svc::DebugExceptionType::UserBreak:
                fprintf(f_report, "    Break Reason:                0x%x\n", this->exception_info.specific.user_break.break_reason);
                fprintf(f_report, "    Break Address:               %s\n", this->module_list.GetFormattedAddressString(this->exception_info.specific.user_break.address));
                fprintf(f_report, "    Break Size:                  0x%lx\n", this->exception_info.specific.user_break.size);
                break;
            default:
                break;
        }

        /* Crashed Thread Info. */
        fprintf(f_report, "Crashed Thread Info:\n");
        this->crashed_thread.SaveToFile(f_report);

        /* Dying Message. */
        if (hos::GetVersion() >= hos::Version_500 && this->dying_message_size != 0) {
            fprintf(f_report, "Dying Message Info:\n");
            fprintf(f_report, "    Address:                     0x%s\n", this->module_list.GetFormattedAddressString(this->dying_message_address));
            fprintf(f_report, "    Size:                        0x%016lx\n", this->dying_message_size);
            DumpMemoryHexToFile(f_report, "    Dying Message:              ", this->dying_message, this->dying_message_size);
        }

        /* Module Info. */
        fprintf(f_report, "Module Info:\n");
        this->module_list.SaveToFile(f_report);

        /* Thread Info. */
        fprintf(f_report, "\nThread Report:\n");
        this->thread_list.SaveToFile(f_report);
    }

}
