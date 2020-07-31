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

        constexpr inline int32_t MaximumDebuggableThreadCount = 0x60;

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

        Result ContinueDebugEventImpl(ams::svc::Handle debug_handle, uint32_t flags, const uint64_t *thread_ids, int32_t num_thread_ids) {
            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Continue the event. */
            R_TRY(debug->ContinueDebug(flags, thread_ids, num_thread_ids));

            return ResultSuccess();
        }

        Result ContinueDebugEvent(ams::svc::Handle debug_handle, uint32_t flags, KUserPointer<const uint64_t *> user_thread_ids, int32_t num_thread_ids) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Verify that the flags are valid. */
            R_UNLESS((flags | ams::svc::ContinueFlag_AllMask) == ams::svc::ContinueFlag_AllMask, svc::ResultInvalidEnumValue());

            /* Verify that continue all and continue others flags are exclusive. */
            constexpr u32 AllAndOthersMask = ams::svc::ContinueFlag_ContinueAll | ams::svc::ContinueFlag_ContinueOthers;
            R_UNLESS((flags & AllAndOthersMask) != AllAndOthersMask, svc::ResultInvalidEnumValue());

            /* Verify that the number of thread ids is valid. */
            R_UNLESS((0 <= num_thread_ids && num_thread_ids <= MaximumDebuggableThreadCount), svc::ResultOutOfRange());

            /* Copy the threads from userspace. */
            uint64_t thread_ids[MaximumDebuggableThreadCount];
            if (num_thread_ids > 0) {
                R_TRY(user_thread_ids.CopyArrayTo(thread_ids, num_thread_ids));
            }

            /* Continue the event. */
            R_TRY(ContinueDebugEventImpl(debug_handle, flags, thread_ids, num_thread_ids));

            return ResultSuccess();
        }

        Result LegacyContinueDebugEvent(ams::svc::Handle debug_handle, uint32_t flags, uint64_t thread_id) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Verify that the flags are valid. */
            R_UNLESS((flags | ams::svc::ContinueFlag_AllMask) == ams::svc::ContinueFlag_AllMask, svc::ResultInvalidEnumValue());

            /* Verify that continue all and continue others flags are exclusive. */
            constexpr u32 AllAndOthersMask = ams::svc::ContinueFlag_ContinueAll | ams::svc::ContinueFlag_ContinueOthers;
            R_UNLESS((flags & AllAndOthersMask) != AllAndOthersMask, svc::ResultInvalidEnumValue());

            /* Continue the event. */
            R_TRY(ContinueDebugEventImpl(debug_handle, flags, std::addressof(thread_id), 1));

            return ResultSuccess();
        }

        Result QueryDebugProcessMemory(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle debug_handle, uintptr_t address) {
            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Query the mapping's info. */
            R_TRY(debug->QueryMemoryInfo(out_memory_info, out_page_info, address));

            return ResultSuccess();
        }

        template<typename T>
        Result QueryDebugProcessMemory(KUserPointer<T *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle debug_handle, uint64_t address) {
            /* Get an ams::svc::MemoryInfo for the region. */
            ams::svc::MemoryInfo info = {};
            R_TRY(QueryDebugProcessMemory(std::addressof(info), out_page_info, debug_handle, address));

            /* Copy the info to userspace. */
            if constexpr (std::same_as<T, ams::svc::MemoryInfo>) {
                R_TRY(out_memory_info.CopyFrom(std::addressof(info)));
            } else {
                /* Convert the info. */
                T converted_info = {};
                static_assert(std::same_as<decltype(T{}.addr), decltype(ams::svc::MemoryInfo{}.addr)>);
                static_assert(std::same_as<decltype(T{}.size), decltype(ams::svc::MemoryInfo{}.size)>);

                converted_info.addr             = info.addr;
                converted_info.size             = info.size;
                converted_info.state            = info.state;
                converted_info.attr             = info.attr;
                converted_info.perm             = info.perm;
                converted_info.ipc_refcount     = info.ipc_refcount;
                converted_info.device_refcount  = info.device_refcount;

                /* Copy it. */
                R_TRY(out_memory_info.CopyFrom(std::addressof(converted_info)));
            }

            return ResultSuccess();
        }

        Result ReadDebugProcessMemory(uintptr_t buffer, ams::svc::Handle debug_handle, uintptr_t address, size_t size) {
            /* Validate address / size. */
            R_UNLESS(size > 0,                   svc::ResultInvalidSize());
            R_UNLESS((address < address + size), svc::ResultInvalidCurrentMemory());
            R_UNLESS((buffer  < buffer  + size), svc::ResultInvalidCurrentMemory());

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Read the memory. */
            R_TRY(debug->ReadMemory(buffer, address, size));

            return ResultSuccess();
        }

        Result WriteDebugProcessMemory(ams::svc::Handle debug_handle, uintptr_t buffer, uintptr_t address, size_t size) {
            /* Validate address / size. */
            R_UNLESS(size > 0,                   svc::ResultInvalidSize());
            R_UNLESS((address < address + size), svc::ResultInvalidCurrentMemory());
            R_UNLESS((buffer  < buffer  + size), svc::ResultInvalidCurrentMemory());

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Write the memory. */
            R_TRY(debug->WriteMemory(buffer, address, size));

            return ResultSuccess();
        }

        Result SetHardwareBreakPoint(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Set the breakpoint. */
            R_TRY(KDebug::SetHardwareBreakPoint(name, flags, value));

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
        return ContinueDebugEvent(debug_handle, flags, thread_ids, num_thread_ids);
    }

    Result LegacyContinueDebugEvent64(ams::svc::Handle debug_handle, uint32_t flags, uint64_t thread_id) {
        return LegacyContinueDebugEvent(debug_handle, flags, thread_id);
    }

    Result GetDebugThreadContext64(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugThreadContext64 was called.");
    }

    Result SetDebugThreadContext64(ams::svc::Handle debug_handle, uint64_t thread_id, KUserPointer<const ams::svc::ThreadContext *> context, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcSetDebugThreadContext64 was called.");
    }

    Result QueryDebugProcessMemory64(KUserPointer<ams::svc::lp64::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle debug_handle, ams::svc::Address address) {
        return QueryDebugProcessMemory(out_memory_info, out_page_info, debug_handle, address);
    }

    Result ReadDebugProcessMemory64(ams::svc::Address buffer, ams::svc::Handle debug_handle, ams::svc::Address address, ams::svc::Size size) {
        return ReadDebugProcessMemory(buffer, debug_handle, address, size);
    }

    Result WriteDebugProcessMemory64(ams::svc::Handle debug_handle, ams::svc::Address buffer, ams::svc::Address address, ams::svc::Size size) {
        return WriteDebugProcessMemory(debug_handle, buffer, address, size);
    }

    Result SetHardwareBreakPoint64(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
        return SetHardwareBreakPoint(name, flags, value);
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
        return ContinueDebugEvent(debug_handle, flags, thread_ids, num_thread_ids);
    }

    Result LegacyContinueDebugEvent64From32(ams::svc::Handle debug_handle, uint32_t flags, uint64_t thread_id) {
        return LegacyContinueDebugEvent(debug_handle, flags, thread_id);
    }

    Result GetDebugThreadContext64From32(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugThreadContext64From32 was called.");
    }

    Result SetDebugThreadContext64From32(ams::svc::Handle debug_handle, uint64_t thread_id, KUserPointer<const ams::svc::ThreadContext *> context, uint32_t context_flags) {
        MESOSPHERE_PANIC("Stubbed SvcSetDebugThreadContext64From32 was called.");
    }

    Result QueryDebugProcessMemory64From32(KUserPointer<ams::svc::ilp32::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle debug_handle, ams::svc::Address address) {
        return QueryDebugProcessMemory(out_memory_info, out_page_info, debug_handle, address);
    }

    Result ReadDebugProcessMemory64From32(ams::svc::Address buffer, ams::svc::Handle debug_handle, ams::svc::Address address, ams::svc::Size size) {
        return ReadDebugProcessMemory(buffer, debug_handle, address, size);
    }

    Result WriteDebugProcessMemory64From32(ams::svc::Handle debug_handle, ams::svc::Address buffer, ams::svc::Address address, ams::svc::Size size) {
        return WriteDebugProcessMemory(debug_handle, buffer, address, size);
    }

    Result SetHardwareBreakPoint64From32(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
        return SetHardwareBreakPoint(name, flags, value);
    }

    Result GetDebugThreadParam64From32(uint64_t *out_64, uint32_t *out_32, ams::svc::Handle debug_handle, uint64_t thread_id, ams::svc::DebugThreadParam param) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugThreadParam64From32 was called.");
    }

}
