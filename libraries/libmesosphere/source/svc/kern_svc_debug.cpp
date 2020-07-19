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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        Result DebugActiveProcess(ams::svc::Handle *out_handle, uint64_t process_id) {
            /* Get the process from its id. */
            KProcess *process = KProcess::GetProcessFromId(process_id);
            R_UNLESS(process != nullptr, svc::ResultInvalidProcessId());

            /* Close the reference we opened to the process on scope exit. */
            ON_SCOPE_EXIT { process->Close(); };

            /* Check that the debugging is allowed. */
            if (!process->IsPermittedDebug()) {
                R_UNLESS(GetCurrentProcess().CanForceDebug(), svc::ResultInvalidState());
            }

            /* Disallow debugging one's own processs, to prevent softlocks. */
            R_UNLESS(process != GetCurrentProcessPointer(), svc::ResultInvalidState());

            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Create a new debug object. */
            KDebug *debug = KDebug::Create();
            R_UNLESS(debug != nullptr, svc::ResultOutOfResource());
            ON_SCOPE_EXIT { debug->Close(); };

            /* Initialize the debug object. */
            debug->Initialize();

            /* Register the debug object. */
            KDebug::Register(debug);

            /* Try to attach to the target process. */
            R_TRY(debug->Attach(process));

            /* Add the new debug object to the handle table. */
            R_TRY(handle_table.Add(out_handle, debug));

            return ResultSuccess();
        }

        template<typename EventInfoType>
        Result GetDebugEvent(KUserPointer<EventInfoType *> out_info, ams::svc::Handle debug_handle) {
            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Create and clear a new event info. */
            EventInfoType info;
            std::memset(std::addressof(info), 0, sizeof(info));

            /* Get the next info from the debug object. */
            R_TRY(debug->GetDebugEventInfo(std::addressof(info)));

            /* Copy the info out to the user. */
            R_TRY(out_info.CopyFrom(std::addressof(info)));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result DebugActiveProcess64(ams::svc::Handle *out_handle, uint64_t process_id) {
        return DebugActiveProcess(out_handle, process_id);
    }

    Result BreakDebugProcess64(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcBreakDebugProcess64 was called.");
    }

    Result TerminateDebugProcess64(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcTerminateDebugProcess64 was called.");
    }

    Result GetDebugEvent64(KUserPointer<ams::svc::lp64::DebugEventInfo *> out_info, ams::svc::Handle debug_handle) {
        return GetDebugEvent(out_info, debug_handle);
    }

    Result ContinueDebugEvent64(ams::svc::Handle debug_handle, uint32_t flags, KUserPointer<const uint64_t *> thread_ids, int32_t num_thread_ids) {
        MESOSPHERE_PANIC("Stubbed SvcContinueDebugEvent64 was called.");
    }

    Result LegacyContinueDebugEvent64(ams::svc::Handle debug_handle, uint32_t flags, uint64_t thread_id) {
        MESOSPHERE_PANIC("Stubbed SvcLegacyContinueDebugEvent64 was called.");
    }

    Result GetDebugThreadContext64(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugThreadContext64 was called.");
    }

    Result SetDebugThreadContext64(ams::svc::Handle debug_handle, uint64_t thread_id, KUserPointer<const ams::svc::ThreadContext *> context, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcSetDebugThreadContext64 was called.");
    }

    Result QueryDebugProcessMemory64(KUserPointer<ams::svc::lp64::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, ams::svc::Address address) {
        MESOSPHERE_PANIC("Stubbed SvcQueryDebugProcessMemory64 was called.");
    }

    Result ReadDebugProcessMemory64(ams::svc::Address buffer, ams::svc::Handle debug_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcReadDebugProcessMemory64 was called.");
    }

    Result WriteDebugProcessMemory64(ams::svc::Handle debug_handle, ams::svc::Address buffer, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcWriteDebugProcessMemory64 was called.");
    }

    Result SetHardwareBreakPoint64(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
        MESOSPHERE_PANIC("Stubbed SvcSetHardwareBreakPoint64 was called.");
    }

    Result GetDebugThreadParam64(uint64_t *out_64, uint32_t *out_32, ams::svc::Handle debug_handle, uint64_t thread_id, ams::svc::DebugThreadParam param) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugThreadParam64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result DebugActiveProcess64From32(ams::svc::Handle *out_handle, uint64_t process_id) {
        return DebugActiveProcess(out_handle, process_id);
    }

    Result BreakDebugProcess64From32(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcBreakDebugProcess64From32 was called.");
    }

    Result TerminateDebugProcess64From32(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcTerminateDebugProcess64From32 was called.");
    }

    Result GetDebugEvent64From32(KUserPointer<ams::svc::ilp32::DebugEventInfo *> out_info, ams::svc::Handle debug_handle) {
        return GetDebugEvent(out_info, debug_handle);
    }

    Result ContinueDebugEvent64From32(ams::svc::Handle debug_handle, uint32_t flags, KUserPointer<const uint64_t *> thread_ids, int32_t num_thread_ids) {
        MESOSPHERE_PANIC("Stubbed SvcContinueDebugEvent64From32 was called.");
    }

    Result LegacyContinueDebugEvent64From32(ams::svc::Handle debug_handle, uint32_t flags, uint64_t thread_id) {
        MESOSPHERE_PANIC("Stubbed SvcLegacyContinueDebugEvent64From32 was called.");
    }

    Result GetDebugThreadContext64From32(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugThreadContext64From32 was called.");
    }

    Result SetDebugThreadContext64From32(ams::svc::Handle debug_handle, uint64_t thread_id, KUserPointer<const ams::svc::ThreadContext *> context, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcSetDebugThreadContext64From32 was called.");
    }

    Result QueryDebugProcessMemory64From32(KUserPointer<ams::svc::ilp32::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, ams::svc::Address address) {
        MESOSPHERE_PANIC("Stubbed SvcQueryDebugProcessMemory64From32 was called.");
    }

    Result ReadDebugProcessMemory64From32(ams::svc::Address buffer, ams::svc::Handle debug_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcReadDebugProcessMemory64From32 was called.");
    }

    Result WriteDebugProcessMemory64From32(ams::svc::Handle debug_handle, ams::svc::Address buffer, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcWriteDebugProcessMemory64From32 was called.");
    }

    Result SetHardwareBreakPoint64From32(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
        MESOSPHERE_PANIC("Stubbed SvcSetHardwareBreakPoint64From32 was called.");
    }

    Result GetDebugThreadParam64From32(uint64_t *out_64, uint32_t *out_32, ams::svc::Handle debug_handle, uint64_t thread_id, ams::svc::DebugThreadParam param) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugThreadParam64From32 was called.");
    }

}
