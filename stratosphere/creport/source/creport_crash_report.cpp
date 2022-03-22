/*
 * Copyright (c) Atmosphère-NX
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
        static_assert(DyingMessageAddressOffset == AMS_OFFSETOF(ams::svc::aarch64::ProcessLocalRegion, dying_message_region_address));
        static_assert(DyingMessageAddressOffset == AMS_OFFSETOF(ams::svc::aarch32::ProcessLocalRegion, dying_message_region_address));

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
                if (R_FAILED(::timeInitialize())) {
                    return false;
                }
                ON_SCOPE_EXIT { ::timeExit(); };

                return R_SUCCEEDED(::timeGetCurrentTime(TimeType_LocalSystemClock, out));
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
        m_heap_handle = lmem::CreateExpHeap(m_heap_storage, sizeof(m_heap_storage), lmem::CreateOption_None);

        /* Allocate members. */
        m_module_list   = std::construct_at(static_cast<ModuleList *>(lmem::AllocateFromExpHeap(m_heap_handle, sizeof(ModuleList))));
        m_thread_list   = std::construct_at(static_cast<ThreadList *>(lmem::AllocateFromExpHeap(m_heap_handle, sizeof(ThreadList))));
        m_dying_message = static_cast<u8 *>(lmem::AllocateFromExpHeap(m_heap_handle, DyingMessageSizeMax));
        if (m_dying_message != nullptr) {
            std::memset(m_dying_message, 0, DyingMessageSizeMax);
        }
    }

    void CrashReport::BuildReport(os::ProcessId process_id, bool has_extra_info) {
        m_has_extra_info = has_extra_info;

        if (this->OpenProcess(process_id)) {
            /* Parse info from the crashed process. */
            this->ProcessExceptions();
            m_module_list->FindModulesFromThreadInfo(m_debug_handle, m_crashed_thread, this->Is64Bit());
            m_thread_list->ReadFromProcess(m_debug_handle, m_thread_tls_map, this->Is64Bit());

            /* Associate module list to threads. */
            m_crashed_thread.SetModuleList(m_module_list);
            m_thread_list->SetModuleList(m_module_list);

            /* Process dying message for applications. */
            if (this->IsApplication()) {
                this->ProcessDyingMessage();
            }

            /* Nintendo's creport finds extra modules by looking at all threads if application, */
            /* but there's no reason for us not to always go looking. */
            for (size_t i = 0; i < m_thread_list->GetThreadCount(); i++) {
                m_module_list->FindModulesFromThreadInfo(m_debug_handle, m_thread_list->GetThreadInfo(i), this->Is64Bit());
            }

            /* Cache the module base address to send to fatal. */
            if (m_module_list->GetModuleCount()) {
                m_module_base_address = m_module_list->GetModuleStartAddress(0);
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
        out->type = static_cast<u32>(m_exception_info.type);

        for (size_t i = 0; i < fatal::aarch64::RegisterName_FP; i++) {
            out->aarch64_ctx.SetRegisterValue(static_cast<fatal::aarch64::RegisterName>(i), m_crashed_thread.GetGeneralPurposeRegister(i));
        }
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_FP, m_crashed_thread.GetFP());
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_LR, m_crashed_thread.GetLR());
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_SP, m_crashed_thread.GetSP());
        out->aarch64_ctx.SetRegisterValue(fatal::aarch64::RegisterName_PC, m_crashed_thread.GetPC());

        out->aarch64_ctx.stack_trace_size = m_crashed_thread.GetStackTraceSize();
        for (size_t i = 0; i < out->aarch64_ctx.stack_trace_size; i++) {
            out->aarch64_ctx.stack_trace[i] = m_crashed_thread.GetStackTrace(i);
        }

        if (m_module_base_address != 0) {
            out->aarch64_ctx.SetBaseAddress(m_module_base_address);
        }

        /* For ams fatal, which doesn't use afsr0, pass program_id instead. */
        out->aarch64_ctx.SetProgramIdForAtmosphere(ncm::ProgramId{m_process_info.program_id});
    }

    void CrashReport::ProcessExceptions() {
        /* Loop all debug events. */
        svc::DebugEventInfo d;
        while (R_SUCCEEDED(svc::GetDebugEvent(std::addressof(d), m_debug_handle))) {
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
        m_crashed_thread.ReadFromProcess(m_debug_handle, m_thread_tls_map, m_crashed_thread_id, this->Is64Bit());
    }

    void CrashReport::HandleDebugEventInfoCreateProcess(const svc::DebugEventInfo &d) {
        m_process_info = d.info.create_process;

        /* On 5.0.0+, we want to parse out a dying message from application crashes. */
        if (hos::GetVersion() < hos::Version_5_0_0 || !IsApplication()) {
            return;
        }

        /* Parse out user data. */
        const u64 address = m_process_info.user_exception_context_address + DyingMessageAddressOffset;
        u64 userdata_address = 0;
        u64 userdata_size = 0;

        /* Read userdata address. */
        if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(userdata_address)), m_debug_handle, address, sizeof(userdata_address)))) {
            return;
        }

        /* Validate userdata address. */
        if (userdata_address == 0 || userdata_address & 0xFFF) {
            return;
        }

        /* Read userdata size. */
        if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(userdata_size)), m_debug_handle, address + sizeof(userdata_address), sizeof(userdata_size)))) {
            return;
        }

        /* Cap userdata size. */
        userdata_size = std::min(size_t(userdata_size), DyingMessageSizeMax);

        m_dying_message_address = userdata_address;
        m_dying_message_size = userdata_size;
    }

    void CrashReport::HandleDebugEventInfoCreateThread(const svc::DebugEventInfo &d) {
        /* Save info on the thread's TLS address for later. */
        m_thread_tls_map.SetThreadTls(d.info.create_thread.thread_id, d.info.create_thread.tls_address);
    }

    void CrashReport::HandleDebugEventInfoException(const svc::DebugEventInfo &d) {
        switch (d.info.exception.type) {
            case svc::DebugException_UndefinedInstruction:
                m_result = creport::ResultUndefinedInstruction();
                break;
            case svc::DebugException_InstructionAbort:
                m_result = creport::ResultInstructionAbort();
                break;
            case svc::DebugException_DataAbort:
                m_result = creport::ResultDataAbort();
                break;
            case svc::DebugException_AlignmentFault:
                m_result = creport::ResultAlignmentFault();
                break;
            case svc::DebugException_UserBreak:
                m_result = creport::ResultUserBreak();
                /* Try to parse out the user break result. */
                if (hos::GetVersion() >= hos::Version_5_0_0) {
                    svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(m_result)), m_debug_handle, d.info.exception.specific.user_break.address, sizeof(m_result));
                }
                break;
            case svc::DebugException_UndefinedSystemCall:
                m_result = creport::ResultUndefinedSystemCall();
                break;
            case svc::DebugException_MemorySystemError:
                m_result = creport::ResultMemorySystemError();
                break;
            case svc::DebugException_DebuggerAttached:
            case svc::DebugException_BreakPoint:
            case svc::DebugException_DebuggerBreak:
                return;
        }

        /* Save exception info. */
        m_exception_info = d.info.exception;
        m_crashed_thread_id = d.thread_id;
    }

    void CrashReport::ProcessDyingMessage() {
        /* Dying message is only stored starting in 5.0.0. */
        if (hos::GetVersion() < hos::Version_5_0_0) {
            return;
        }

        /* Validate address/size. */
        if (m_dying_message_address == 0 || m_dying_message_address & 0xFFF) {
            return;
        }
        if (m_dying_message_size > DyingMessageSizeMax) {
            return;
        }

        /* Validate that the current report isn't garbage. */
        if (!IsOpen() || !IsComplete()) {
            return;
        }

        /* Verify that we have a dying message buffer. */
        if (m_dying_message == nullptr) {
            return;
        }

        /* Read the dying message. */
        svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(m_dying_message), m_debug_handle, m_dying_message_address, m_dying_message_size);
    }

    void CrashReport::SaveReport(bool enable_screenshot) {
        /* Try to ensure path exists. */
        TryCreateReportDirectories();

        /* Get a timestamp. */
        u64 timestamp;
        if (!TryGetCurrentTimestamp(&timestamp)) {
            timestamp = os::GetSystemTick().GetInt64Value();
        }

        /* Save files. */
        {
            char file_path[fs::EntryNameLengthMax + 1];

            /* Save crash report. */
            util::SNPrintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/%011lu_%016lx.log", timestamp, m_process_info.program_id);
            {
                ScopedFile file(file_path);
                if (file.IsOpen()) {
                    this->SaveToFile(file);
                }
            }

            /* Dump threads. */
            util::SNPrintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/dumps/%011lu_%016lx_thread_info.bin", timestamp, m_process_info.program_id);
            {
                ScopedFile file(file_path);
                if (file.IsOpen()) {
                    m_thread_list->DumpBinary(file, m_crashed_thread.GetThreadId());
                }
            }

            /* If we're open, we need to close here. */
            if (this->IsOpen()) {
                this->Close();
            }

            /* Finalize our heap. */
            std::destroy_at(m_module_list);
            std::destroy_at(m_thread_list);
            lmem::FreeToExpHeap(m_heap_handle, m_module_list);
            lmem::FreeToExpHeap(m_heap_handle, m_thread_list);
            if (m_dying_message != nullptr) {
                lmem::FreeToExpHeap(m_heap_handle, m_dying_message);
            }
            m_module_list   = nullptr;
            m_thread_list   = nullptr;
            m_dying_message = nullptr;

            /* Try to take a screenshot. */
            /* NOTE: Nintendo validates that enable_screenshot is true here, and validates that the application id is not in a blacklist. */
            /* Since we save reports only locally and do not send them via telemetry, we will skip this. */
            AMS_UNUSED(enable_screenshot);
            if (hos::GetVersion() >= hos::Version_9_0_0 && this->IsApplication()) {
                if (R_SUCCEEDED(capsrv::InitializeScreenShotControl())) {
                    ON_SCOPE_EXIT { capsrv::FinalizeScreenShotControl(); };

                    u64 jpeg_size;
                    if (R_SUCCEEDED(capsrv::CaptureJpegScreenshot(std::addressof(jpeg_size), m_heap_storage, sizeof(m_heap_storage), vi::LayerStack_ApplicationForDebug, TimeSpan::FromSeconds(10)))) {
                        util::SNPrintf(file_path, sizeof(file_path), "sdmc:/atmosphere/crash_reports/%011lu_%016lx.jpg", timestamp, m_process_info.program_id);
                        ScopedFile file(file_path);
                        if (file.IsOpen()) {
                            file.Write(m_heap_storage, jpeg_size);
                        }
                    }
                }
            }
        }
    }

    void CrashReport::SaveToFile(ScopedFile &file) {
        file.WriteFormat("Atmosphère Crash Report (v1.7):\n");

        file.WriteFormat("Result:                          0x%X (2%03d-%04d)\n\n", m_result.GetValue(), m_result.GetModule(), m_result.GetDescription());

        /* Process Info. */
        char name_buf[0x10] = {};
        static_assert(sizeof(name_buf) >= sizeof(m_process_info.name), "buffer overflow!");
        std::memcpy(name_buf, m_process_info.name, sizeof(m_process_info.name));
        file.WriteFormat("Process Info:\n");
        file.WriteFormat("    Process Name:                %s\n", name_buf);
        file.WriteFormat("    Program ID:                  %016lx\n", m_process_info.program_id);
        file.WriteFormat("    Process ID:                  %016lx\n", m_process_info.process_id);
        file.WriteFormat("    Process Flags:               %08x\n", m_process_info.flags);
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            file.WriteFormat("    User Exception Address:      %s\n", m_module_list->GetFormattedAddressString(m_process_info.user_exception_context_address));
        }

        /* Exception Info. */
        file.WriteFormat("Exception Info:\n");
        file.WriteFormat("    Type:                        %s\n", GetDebugExceptionString(m_exception_info.type));
        file.WriteFormat("    Address:                     %s\n", m_module_list->GetFormattedAddressString(m_exception_info.address));
        switch (m_exception_info.type) {
            case svc::DebugException_UndefinedInstruction:
                file.WriteFormat("    Opcode:                      %08x\n", m_exception_info.specific.undefined_instruction.insn);
                break;
            case svc::DebugException_DataAbort:
            case svc::DebugException_AlignmentFault:
                if (m_exception_info.specific.raw != m_exception_info.address) {
                    file.WriteFormat("    Fault Address:               %s\n", m_module_list->GetFormattedAddressString(m_exception_info.specific.raw));
                }
                break;
            case svc::DebugException_UndefinedSystemCall:
                file.WriteFormat("    Svc Id:                      0x%02x\n", m_exception_info.specific.undefined_system_call.id);
                break;
            case svc::DebugException_UserBreak:
                file.WriteFormat("    Break Reason:                0x%x\n", m_exception_info.specific.user_break.break_reason);
                file.WriteFormat("    Break Address:               %s\n", m_module_list->GetFormattedAddressString(m_exception_info.specific.user_break.address));
                file.WriteFormat("    Break Size:                  0x%lx\n", m_exception_info.specific.user_break.size);
                break;
            default:
                break;
        }

        /* Crashed Thread Info. */
        file.WriteFormat("Crashed Thread Info:\n");
        m_crashed_thread.SaveToFile(file);

        /* Dying Message. */
        if (hos::GetVersion() >= hos::Version_5_0_0 && m_dying_message_size != 0) {
            file.WriteFormat("Dying Message Info:\n");
            file.WriteFormat("    Address:                     0x%s\n", m_module_list->GetFormattedAddressString(m_dying_message_address));
            file.WriteFormat("    Size:                        0x%016lx\n", m_dying_message_size);
            file.DumpMemory( "    Dying Message:               ", m_dying_message, m_dying_message_size);
        }

        /* Module Info. */
        file.WriteFormat("Module Info:\n");
        m_module_list->SaveToFile(file);

        /* Thread Info. */
        file.WriteFormat("Thread Report:\n");
        m_thread_list->SaveToFile(file);
    }

}
