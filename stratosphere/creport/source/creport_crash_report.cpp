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
#include <stratosphere.hpp>
#include "creport_crash_report.hpp"
#include "creport_utils.hpp"

namespace ams::creport {

    namespace {

        /* Convenience definitions. */
        constexpr size_t DyingMessageAddressOffset = 0x1C0;
        static_assert(DyingMessageAddressOffset == OFFSETOF(ams::svc::aarch64::ProcessLocalRegion, dying_message_region_address));
        static_assert(DyingMessageAddressOffset == OFFSETOF(ams::svc::aarch32::ProcessLocalRegion, dying_message_region_address));

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
            fs::EnsureDirectoryRecursively("sdmc:/atmosphere/crash_reports/dumps");
            fs::EnsureDirectoryRecursively("sdmc:/atmosphere/fatal_reports/dumps");
        }

        constexpr const char *GetDebugExceptionString(const svc::DebugException type) {
            switch (type) {
                case svc::DebugException_UndefinedInstruction:
                    return "Undefined Instruction";
                case svc::DebugException_InstructionAbort:
                    return "Instruction Abort";
                case svc::DebugException_DataAbort:
                    return "Data Abort";
                case svc::DebugException_AlignmentFault:
                    return "Alignment Fault";
                case svc::DebugException_DebuggerAttached:
                    return "Debugger Attached";
                case svc::DebugException_BreakPoint:
                    return "Break Point";
                case svc::DebugException_UserBreak:
                    return "User Break";
                case svc::DebugException_DebuggerBreak:
                    return "Debugger Break";
                case svc::DebugException_UndefinedSystemCall:
                    return "Undefined System Call";
                case svc::DebugException_MemorySystemError:
                    return "System Memory Error";
                default:
                    return "Unknown";
            }
        }

    }

    void CrashReport::Initialize() {
        /* Initialize the heap. */
        this->heap_handle = lmem::CreateExpHeap(this->heap_storage, sizeof(this->heap_storage), lmem::CreateOption_None);

        /* Allocate members. */
        this->module_list   = new (lmem::AllocateFromExpHeap(this->heap_handle, sizeof(ModuleList))) ModuleList;
        this->thread_list   = new (lmem::AllocateFromExpHeap(this->heap_handle, sizeof(ThreadList))) ThreadList;
        this->dying_message = static_cast<u8 *>(lmem::AllocateFromExpHeap(this->heap_handle, DyingMessageSizeMax));
        if (this->dying_message != nullptr) {
            std::memset(this->dying_message, 0, DyingMessageSizeMax);
        }
    }

    void CrashReport::BuildReport(os::ProcessId process_id, bool has_extra_info) {
        this->has_extra_info = has_extra_info;

        if (this->OpenProcess(process_id)) {
            ON_SCOPE_EXIT { this->Close(); };

            /* Parse info from the crashed process. */
            this->ProcessExceptions();
            this->module_list->FindModulesFromThreadInfo(this->debug_handle, this->crashed_thread);
            this->thread_list->ReadFromProcess(this->debug_handle, this->thread_tls_map, this->Is64Bit());

            /* Associate module list to threads. */
            this->crashed_thread.SetModuleList(this->module_list);
            this->thread_list->SetModuleList(this->module_list);

            /* Process dying message for applications. */
            if (this->IsApplication()) {
                this->ProcessDyingMessage();
            }

            /* Nintendo's creport finds extra modules by looking at all threads if application, */
            /* but there's no reason for us not to always go looking. */
            for (size_t i = 0; i < this->thread_list->GetThreadCount(); i++) {
                this->module_list->FindModulesFromThreadInfo(this->debug_handle, this->thread_list->GetThreadInfo(i));
            }

            /* Cache the module base address to send to fatal. */
            if (this->module_list->GetModuleCount()) {
                this->module_base_address = this->module_list->GetModuleStartAddress(0);
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

        if (this->module_base_address != 0) {
            out->aarch64_ctx.SetBaseAddress(this->module_base_address);
        }

        /* For ams fatal, which doesn't use afsr0, pass program_id instead. */
        out->aarch64_ctx.SetProgramIdForAtmosphere(ncm::ProgramId{this->process_info.program_id});
    }

    void CrashReport::ProcessExceptions() {
        /* Loop all debug events. */
        svc::DebugEventInfo d;
        while (R_SUCCEEDED(svcGetDebugEvent(reinterpret_cast<u8 *>(&d), this->debug_handle))) {
            switch (d.type) {
                case svc::DebugEvent_CreateProcess:
                    this->HandleDebugEventInfoCreateProcess(d);
                    break;
                case svc::DebugEvent_CreateThread:
                    this->HandleDebugEventInfoCreateThread(d);
                    break;
                case svc::DebugEvent_Exception:
                    this->HandleDebugEventInfoException(d);
                    break;
                case svc::DebugEvent_ExitProcess:
                case svc::DebugEvent_ExitThread:
                    break;
            }
        }

        /* Parse crashed thread info. */
        this->crashed_thread.ReadFromProcess(this->debug_handle, this->thread_tls_map, this->crashed_thread_id, this->Is64Bit());
    }

    void CrashReport::HandleDebugEventInfoCreateProcess(const svc::DebugEventInfo &d) {
        this->process_info = d.info.create_process;

        /* On 5.0.0+, we want to parse out a dying message from application crashes. */
        if (hos::GetVersion() < hos::Version_5_0_0 || !IsApplication()) {
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
        userdata_size = std::min(size_t(userdata_size), DyingMessageSizeMax);

        this->dying_message_address = userdata_address;
        this->dying_message_size = userdata_size;
    }

    void CrashReport::HandleDebugEventInfoCreateThread(const svc::DebugEventInfo &d) {
        /* Save info on the thread's TLS address for later. */
        this->thread_tls_map[d.info.create_thread.thread_id] = d.info.create_thread.tls_address;
    }

    void CrashReport::HandleDebugEventInfoException(const svc::DebugEventInfo &d) {
        switch (d.info.exception.type) {
            case svc::DebugException_UndefinedInstruction:
                this->result = ResultUndefinedInstruction();
                break;
            case svc::DebugException_InstructionAbort:
                this->result = ResultInstructionAbort();
                break;
            case svc::DebugException_DataAbort:
                this->result = ResultDataAbort();
                break;
            case svc::DebugException_AlignmentFault:
                this->result = ResultAlignmentFault();
                break;
            case svc::DebugException_UserBreak:
                this->result = ResultUserBreak();
                /* Try to parse out the user break result. */
                if (hos::GetVersion() >= hos::Version_5_0_0) {
                    svcReadDebugProcessMemory(&this->result, this->debug_handle, d.info.exception.specific.user_break.address, sizeof(this->result));
                }
                break;
            case svc::DebugException_UndefinedSystemCall:
                this->result = ResultUndefinedSystemCall();
                break;
            case svc::DebugException_MemorySystemError:
                this->result = ResultMemorySystemError();
                break;
            case svc::DebugException_DebuggerAttached:
            case svc::DebugException_BreakPoint:
            case svc::DebugException_DebuggerBreak:
                return;
        }

        /* Save exception info. */
        this->exception_info = d.info.exception;
        this->crashed_thread_id = d.thread_id;
    }

    void CrashReport::ProcessDyingMessage() {
        /* Dying message is only stored starting in 5.0.0. */
        if (hos::GetVersion() < hos::Version_5_0_0) {
            return;
        }

        /* Validate address/size. */
        if (this->dying_message_address == 0 || this->dying_message_address & 0xFFF) {
            return;
        }
        if (this->dying_message_size > DyingMessageSizeMax) {
            return;
        }

        /* Validate that the current report isn't garbage. */
        if (!IsOpen() || !IsComplete()) {
            return;
        }

        /* Verify that we have a dying message buffer. */
        if (this->dying_message == nullptr) {
            return;
        }

        /* Read the dying message. */
        svcReadDebugProcessMemory(this->dying_message, this->debug_handle, this->dying_message_address, this->dying_message_size);
    }

    void CrashReport::SaveReport(bool enable_screenshot) {
        /* Try to ensure path exists. */
        TryCreateReportDirectories();

        /* Get a timestamp. */
        u64 timestamp;
        if (!TryGetCurrentTimestamp(&timestamp)) {
            timestamp = svcGetSystemTick();
        }

        /* Save files. */
        {
            char file_path[fs::EntryNameLengthMax + 1];

            /* Save crash report. */
            std::snprintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/%011lu_%016lx.log", timestamp, this->process_info.program_id);
            {
                ScopedFile file(file_path);
                if (file.IsOpen()) {
                    this->SaveToFile(file);
                }
            }

            /* Dump threads. */
            std::snprintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/dumps/%011lu_%016lx_thread_info.bin", timestamp, this->process_info.program_id);
            {
                ScopedFile file(file_path);
                if (file.IsOpen()) {
                    this->thread_list->DumpBinary(file, this->crashed_thread.GetThreadId());
                }
            }

            /* Finalize our heap. */
            this->module_list->~ModuleList();
            this->thread_list->~ThreadList();
            lmem::FreeToExpHeap(this->heap_handle, this->module_list);
            lmem::FreeToExpHeap(this->heap_handle, this->thread_list);
            if (this->dying_message != nullptr) {
                lmem::FreeToExpHeap(this->heap_handle, this->dying_message);
            }
            this->module_list   = nullptr;
            this->thread_list   = nullptr;
            this->dying_message = nullptr;

            /* Try to take a screenshot. */
            /* NOTE: Nintendo validates that enable_screenshot is true here, and validates that the application id is not in a blacklist. */
            /* Since we save reports only locally and do not send them via telemetry, we will skip this. */
            AMS_UNUSED(enable_screenshot);
            if (hos::GetVersion() >= hos::Version_9_0_0 && this->IsApplication()) {
                sm::ScopedServiceHolder<capsrv::InitializeScreenShotControl, capsrv::FinalizeScreenShotControl> capssc_holder;
                if (capssc_holder) {
                    u64 jpeg_size;
                    if (R_SUCCEEDED(capsrv::CaptureJpegScreenshot(std::addressof(jpeg_size), this->heap_storage, sizeof(this->heap_storage), vi::LayerStack_ApplicationForDebug, TimeSpan::FromSeconds(10)))) {
                        std::snprintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/%011lu_%016lx.jpg", timestamp, this->process_info.program_id);
                        ScopedFile file(file_path);
                        if (file.IsOpen()) {
                            file.Write(this->heap_storage, jpeg_size);
                        }
                    }
                }
            }
        }
    }

    void CrashReport::SaveToFile(ScopedFile &file) {
        file.WriteFormat("Atmosphère Crash Report (v1.5):\n");

        /* TODO: Remove in Atmosphere 1.0.0. */
        file.WriteFormat("Mesosphere:                      %s\n", svc::IsKernelMesosphere() ? "Enabled" : "Disabled");

        file.WriteFormat("Result:                          0x%X (2%03d-%04d)\n\n", this->result.GetValue(), this->result.GetModule(), this->result.GetDescription());

        /* Process Info. */
        char name_buf[0x10] = {};
        static_assert(sizeof(name_buf) >= sizeof(this->process_info.name), "buffer overflow!");
        std::memcpy(name_buf, this->process_info.name, sizeof(this->process_info.name));
        file.WriteFormat("Process Info:\n");
        file.WriteFormat("    Process Name:                %s\n", name_buf);
        file.WriteFormat("    Program ID:                  %016lx\n", this->process_info.program_id);
        file.WriteFormat("    Process ID:                  %016lx\n", this->process_info.process_id);
        file.WriteFormat("    Process Flags:               %08x\n", this->process_info.flags);
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            file.WriteFormat("    User Exception Address:      %s\n", this->module_list->GetFormattedAddressString(this->process_info.user_exception_context_address));
        }

        /* Exception Info. */
        file.WriteFormat("Exception Info:\n");
        file.WriteFormat("    Type:                        %s\n", GetDebugExceptionString(this->exception_info.type));
        file.WriteFormat("    Address:                     %s\n", this->module_list->GetFormattedAddressString(this->exception_info.address));
        switch (this->exception_info.type) {
            case svc::DebugException_UndefinedInstruction:
                file.WriteFormat("    Opcode:                      %08x\n", this->exception_info.specific.undefined_instruction.insn);
                break;
            case svc::DebugException_DataAbort:
            case svc::DebugException_AlignmentFault:
                if (this->exception_info.specific.raw != this->exception_info.address) {
                    file.WriteFormat("    Fault Address:               %s\n", this->module_list->GetFormattedAddressString(this->exception_info.specific.raw));
                }
                break;
            case svc::DebugException_UndefinedSystemCall:
                file.WriteFormat("    Svc Id:                      0x%02x\n", this->exception_info.specific.undefined_system_call.id);
                break;
            case svc::DebugException_UserBreak:
                file.WriteFormat("    Break Reason:                0x%x\n", this->exception_info.specific.user_break.break_reason);
                file.WriteFormat("    Break Address:               %s\n", this->module_list->GetFormattedAddressString(this->exception_info.specific.user_break.address));
                file.WriteFormat("    Break Size:                  0x%lx\n", this->exception_info.specific.user_break.size);
                break;
            default:
                break;
        }

        /* Crashed Thread Info. */
        file.WriteFormat("Crashed Thread Info:\n");
        this->crashed_thread.SaveToFile(file);

        /* Dying Message. */
        if (hos::GetVersion() >= hos::Version_5_0_0 && this->dying_message_size != 0) {
            file.WriteFormat("Dying Message Info:\n");
            file.WriteFormat("    Address:                     0x%s\n", this->module_list->GetFormattedAddressString(this->dying_message_address));
            file.WriteFormat("    Size:                        0x%016lx\n", this->dying_message_size);
            file.DumpMemory( "    Dying Message:               ", this->dying_message, this->dying_message_size);
        }

        /* Module Info. */
        file.WriteFormat("Module Info:\n");
        this->module_list->SaveToFile(file);

        /* Thread Info. */
        file.WriteFormat("Thread Report:\n");
        this->thread_list->SaveToFile(file);
    }

}
