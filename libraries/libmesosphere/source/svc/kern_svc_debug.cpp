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

            R_SUCCEED();
        }

        Result BreakDebugProcess(ams::svc::Handle debug_handle) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Break the process. */
            R_TRY(debug->BreakProcess());

            R_SUCCEED();
        }

        Result TerminateDebugProcess(ams::svc::Handle debug_handle) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Terminate the process. */
            R_TRY(debug->TerminateProcess());

            R_SUCCEED();
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

            R_SUCCEED();
        }

        Result ContinueDebugEventImpl(ams::svc::Handle debug_handle, uint32_t flags, const uint64_t *thread_ids, int32_t num_thread_ids) {
            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Continue the event. */
            R_TRY(debug->ContinueDebug(flags, thread_ids, num_thread_ids));

            R_SUCCEED();
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

            R_SUCCEED();
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

            R_SUCCEED();
        }

        Result GetDebugThreadContext(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
            /* Validate the context flags. */
            R_UNLESS((context_flags | ams::svc::ThreadContextFlag_All) == ams::svc::ThreadContextFlag_All, svc::ResultInvalidEnumValue());

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the thread context. */
            ams::svc::ThreadContext context = {};
            R_TRY(debug->GetThreadContext(std::addressof(context), thread_id, context_flags));

            /* Copy the context to userspace. */
            R_TRY(out_context.CopyFrom(std::addressof(context)));

            R_SUCCEED();
        }

        Result SetDebugThreadContext(ams::svc::Handle debug_handle, uint64_t thread_id, KUserPointer<const ams::svc::ThreadContext *> user_context, uint32_t context_flags) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Validate the context flags. */
            #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
            {
                /* Check that the flags are a subset of the allowable. */
                constexpr u32 AllFlagsMask = ams::svc::ThreadContextFlag_All | ams::svc::ThreadContextFlag_SetSingleStep | ams::svc::ThreadContextFlag_ClearSingleStep;
                R_UNLESS((context_flags | AllFlagsMask) == AllFlagsMask, svc::ResultInvalidEnumValue());

                /* Check that thread isn't both setting and clearing single step. */
                const bool set_ss   = (context_flags & ams::svc::ThreadContextFlag_SetSingleStep) != 0;
                const bool clear_ss = (context_flags & ams::svc::ThreadContextFlag_ClearSingleStep) != 0;

                R_UNLESS(!(set_ss && clear_ss), svc::ResultInvalidEnumValue());
            }
            #else
            {
                /* Check that the flags are a subset of the allowable. */
                R_UNLESS((context_flags | ams::svc::ThreadContextFlag_All) == ams::svc::ThreadContextFlag_All, svc::ResultInvalidEnumValue());
            }
            #endif

            /* Copy the thread context from userspace. */
            ams::svc::ThreadContext context;
            R_TRY(user_context.CopyTo(std::addressof(context)));

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Set the thread context. */
            R_TRY(debug->SetThreadContext(context, thread_id, context_flags));

            R_SUCCEED();
        }

        Result QueryDebugProcessMemory(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle debug_handle, uintptr_t address) {
            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Query the mapping's info. */
            R_TRY(debug->QueryMemoryInfo(out_memory_info, out_page_info, address));

            R_SUCCEED();
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
                static_assert(std::same_as<decltype(T{}.base_address), decltype(ams::svc::MemoryInfo{}.base_address)>);
                static_assert(std::same_as<decltype(T{}.size), decltype(ams::svc::MemoryInfo{}.size)>);

                converted_info.base_address = info.base_address;
                converted_info.size         = info.size;
                converted_info.state        = info.state;
                converted_info.attribute    = info.attribute;
                converted_info.permission   = info.permission;
                converted_info.ipc_count    = info.ipc_count;
                converted_info.device_count = info.device_count;

                /* Copy it. */
                R_TRY(out_memory_info.CopyFrom(std::addressof(converted_info)));
            }

            R_SUCCEED();
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

            R_SUCCEED();
        }

        Result WriteDebugProcessMemory(ams::svc::Handle debug_handle, uintptr_t buffer, uintptr_t address, size_t size) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Validate address / size. */
            R_UNLESS(size > 0,                   svc::ResultInvalidSize());
            R_UNLESS((address < address + size), svc::ResultInvalidCurrentMemory());
            R_UNLESS((buffer  < buffer  + size), svc::ResultInvalidCurrentMemory());

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Write the memory. */
            R_TRY(debug->WriteMemory(buffer, address, size));

            R_SUCCEED();
        }

        Result SetHardwareBreakPoint(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNotImplemented());

            /* Set the breakpoint. */
            R_TRY(KDebug::SetHardwareBreakPoint(name, flags, value));

            R_SUCCEED();
        }

        Result GetDebugThreadParam(uint64_t *out_64, uint32_t *out_32, ams::svc::Handle debug_handle, uint64_t thread_id, ams::svc::DebugThreadParam param) {
            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the thread from its id. */
            KThread *thread = KThread::GetThreadFromId(thread_id);
            R_UNLESS(thread != nullptr, svc::ResultInvalidThreadId());
            ON_SCOPE_EXIT { thread->Close(); };

            /* Get the process from the debug object. */
            R_UNLESS(debug->IsAttached(),  svc::ResultProcessTerminated());
            R_UNLESS(debug->OpenProcess(), svc::ResultProcessTerminated());

            /* Close the process when we're done. */
            ON_SCOPE_EXIT { debug->CloseProcess(); };

            /* Get the proces. */
            KProcess * const process = debug->GetProcessUnsafe();

            /* Verify that the process is the thread's parent. */
            R_UNLESS(process == thread->GetOwnerProcess(), svc::ResultInvalidThreadId());

            /* Get the parameter. */
            switch (param) {
                case ams::svc::DebugThreadParam_Priority:
                    {
                        /* Get the priority. */
                        *out_32 = thread->GetPriority();
                    }
                    break;
                case ams::svc::DebugThreadParam_State:
                    {
                        /* Get the thread state and suspend status. */
                        KThread::ThreadState state;
                        bool suspended_user;
                        bool suspended_debug;
                        {
                            KScopedSchedulerLock sl;

                            state           = thread->GetState();
                            suspended_user  = thread->IsSuspendRequested(KThread::SuspendType_Thread);
                            suspended_debug = thread->IsSuspendRequested(KThread::SuspendType_Debug);
                        }

                        /* Set the suspend flags. */
                        *out_32 = 0;
                        if (suspended_user) {
                            *out_32 |= ams::svc::ThreadSuspend_User;
                        }
                        if (suspended_debug) {
                            *out_32 |= ams::svc::ThreadSuspend_Debug;
                        }

                        /* Set the state. */
                        switch (state) {
                            case KThread::ThreadState_Initialized:
                                {
                                    *out_64 = ams::svc::ThreadState_Initializing;
                                }
                                break;
                            case KThread::ThreadState_Waiting:
                                {
                                    *out_64 = ams::svc::ThreadState_Waiting;
                                }
                                break;
                            case KThread::ThreadState_Runnable:
                                {
                                    *out_64 = ams::svc::ThreadState_Running;
                                }
                                break;
                            case KThread::ThreadState_Terminated:
                                {
                                    *out_64 = ams::svc::ThreadState_Terminated;
                                }
                                break;
                            default:
                                R_THROW(svc::ResultInvalidState());
                        }
                    }
                    break;
                case ams::svc::DebugThreadParam_IdealCore:
                    {
                        /* Get the ideal core. */
                        s32 core_id;
                        u64 affinity_mask;
                        thread->GetPhysicalCoreMask(std::addressof(core_id), std::addressof(affinity_mask));

                        *out_32 = core_id;
                    }
                    break;
                case ams::svc::DebugThreadParam_CurrentCore:
                    {
                        /* Get the current core. */
                        *out_32 = thread->GetActiveCore();
                    }
                    break;
                case ams::svc::DebugThreadParam_AffinityMask:
                    {
                        /* Get the affinity mask. */
                        s32 core_id;
                        u64 affinity_mask;
                        thread->GetPhysicalCoreMask(std::addressof(core_id), std::addressof(affinity_mask));

                        *out_32 = affinity_mask;
                    }
                    break;
                default:
                    R_THROW(ams::svc::ResultInvalidEnumValue());
            }

            R_SUCCEED();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result DebugActiveProcess64(ams::svc::Handle *out_handle, uint64_t process_id) {
        R_RETURN(DebugActiveProcess(out_handle, process_id));
    }

    Result BreakDebugProcess64(ams::svc::Handle debug_handle) {
        R_RETURN(BreakDebugProcess(debug_handle));
    }

    Result TerminateDebugProcess64(ams::svc::Handle debug_handle) {
        R_RETURN(TerminateDebugProcess(debug_handle));
    }

    Result GetDebugEvent64(KUserPointer<ams::svc::lp64::DebugEventInfo *> out_info, ams::svc::Handle debug_handle) {
        R_RETURN(GetDebugEvent(out_info, debug_handle));
    }

    Result ContinueDebugEvent64(ams::svc::Handle debug_handle, uint32_t flags, KUserPointer<const uint64_t *> thread_ids, int32_t num_thread_ids) {
        R_RETURN(ContinueDebugEvent(debug_handle, flags, thread_ids, num_thread_ids));
    }

    Result LegacyContinueDebugEvent64(ams::svc::Handle debug_handle, uint32_t flags, uint64_t thread_id) {
        R_RETURN(LegacyContinueDebugEvent(debug_handle, flags, thread_id));
    }

    Result GetDebugThreadContext64(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
        R_RETURN(GetDebugThreadContext(out_context, debug_handle, thread_id, context_flags));
    }

    Result SetDebugThreadContext64(ams::svc::Handle debug_handle, uint64_t thread_id, KUserPointer<const ams::svc::ThreadContext *> context, uint32_t context_flags) {
        R_RETURN(SetDebugThreadContext(debug_handle, thread_id, context, context_flags));
    }

    Result QueryDebugProcessMemory64(KUserPointer<ams::svc::lp64::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle debug_handle, ams::svc::Address address) {
        R_RETURN(QueryDebugProcessMemory(out_memory_info, out_page_info, debug_handle, address));
    }

    Result ReadDebugProcessMemory64(ams::svc::Address buffer, ams::svc::Handle debug_handle, ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(ReadDebugProcessMemory(buffer, debug_handle, address, size));
    }

    Result WriteDebugProcessMemory64(ams::svc::Handle debug_handle, ams::svc::Address buffer, ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(WriteDebugProcessMemory(debug_handle, buffer, address, size));
    }

    Result SetHardwareBreakPoint64(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
        R_RETURN(SetHardwareBreakPoint(name, flags, value));
    }

    Result GetDebugThreadParam64(uint64_t *out_64, uint32_t *out_32, ams::svc::Handle debug_handle, uint64_t thread_id, ams::svc::DebugThreadParam param) {
        R_RETURN(GetDebugThreadParam(out_64, out_32, debug_handle, thread_id, param));
    }

    /* ============================= 64From32 ABI ============================= */

    Result DebugActiveProcess64From32(ams::svc::Handle *out_handle, uint64_t process_id) {
        R_RETURN(DebugActiveProcess(out_handle, process_id));
    }

    Result BreakDebugProcess64From32(ams::svc::Handle debug_handle) {
        R_RETURN(BreakDebugProcess(debug_handle));
    }

    Result TerminateDebugProcess64From32(ams::svc::Handle debug_handle) {
        R_RETURN(TerminateDebugProcess(debug_handle));
    }

    Result GetDebugEvent64From32(KUserPointer<ams::svc::ilp32::DebugEventInfo *> out_info, ams::svc::Handle debug_handle) {
        R_RETURN(GetDebugEvent(out_info, debug_handle));
    }

    Result ContinueDebugEvent64From32(ams::svc::Handle debug_handle, uint32_t flags, KUserPointer<const uint64_t *> thread_ids, int32_t num_thread_ids) {
        R_RETURN(ContinueDebugEvent(debug_handle, flags, thread_ids, num_thread_ids));
    }

    Result LegacyContinueDebugEvent64From32(ams::svc::Handle debug_handle, uint32_t flags, uint64_t thread_id) {
        R_RETURN(LegacyContinueDebugEvent(debug_handle, flags, thread_id));
    }

    Result GetDebugThreadContext64From32(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
        R_RETURN(GetDebugThreadContext(out_context, debug_handle, thread_id, context_flags));
    }

    Result SetDebugThreadContext64From32(ams::svc::Handle debug_handle, uint64_t thread_id, KUserPointer<const ams::svc::ThreadContext *> context, uint32_t context_flags) {
        R_RETURN(SetDebugThreadContext(debug_handle, thread_id, context, context_flags));
    }

    Result QueryDebugProcessMemory64From32(KUserPointer<ams::svc::ilp32::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle debug_handle, ams::svc::Address address) {
        R_RETURN(QueryDebugProcessMemory(out_memory_info, out_page_info, debug_handle, address));
    }

    Result ReadDebugProcessMemory64From32(ams::svc::Address buffer, ams::svc::Handle debug_handle, ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(ReadDebugProcessMemory(buffer, debug_handle, address, size));
    }

    Result WriteDebugProcessMemory64From32(ams::svc::Handle debug_handle, ams::svc::Address buffer, ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(WriteDebugProcessMemory(debug_handle, buffer, address, size));
    }

    Result SetHardwareBreakPoint64From32(ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
        R_RETURN(SetHardwareBreakPoint(name, flags, value));
    }

    Result GetDebugThreadParam64From32(uint64_t *out_64, uint32_t *out_32, ams::svc::Handle debug_handle, uint64_t thread_id, ams::svc::DebugThreadParam param) {
        R_RETURN(GetDebugThreadParam(out_64, out_32, debug_handle, thread_id, param));
    }

}
