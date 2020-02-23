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



    }

    /* =============================    64 ABI    ============================= */

    Result DebugActiveProcess64(ams::svc::Handle *out_handle, uint64_t process_id) {
        MESOSPHERE_PANIC("Stubbed SvcDebugActiveProcess64 was called.");
    }

    Result BreakDebugProcess64(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcBreakDebugProcess64 was called.");
    }

    Result TerminateDebugProcess64(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcTerminateDebugProcess64 was called.");
    }

    Result GetDebugEvent64(KUserPointer<ams::svc::lp64::DebugEventInfo *> out_info, ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugEvent64 was called.");
    }

    Result ContinueDebugEvent64(ams::svc::Handle debug_handle, uint32_t flags, KUserPointer<const uint64_t *> thread_ids, int32_t num_thread_ids) {
        MESOSPHERE_PANIC("Stubbed SvcContinueDebugEvent64 was called.");
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
        MESOSPHERE_PANIC("Stubbed SvcDebugActiveProcess64From32 was called.");
    }

    Result BreakDebugProcess64From32(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcBreakDebugProcess64From32 was called.");
    }

    Result TerminateDebugProcess64From32(ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcTerminateDebugProcess64From32 was called.");
    }

    Result GetDebugEvent64From32(KUserPointer<ams::svc::ilp32::DebugEventInfo *> out_info, ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugEvent64From32 was called.");
    }

    Result ContinueDebugEvent64From32(ams::svc::Handle debug_handle, uint32_t flags, KUserPointer<const uint64_t *> thread_ids, int32_t num_thread_ids) {
        MESOSPHERE_PANIC("Stubbed SvcContinueDebugEvent64From32 was called.");
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
