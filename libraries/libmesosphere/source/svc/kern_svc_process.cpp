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

        constexpr bool IsValidCoreId(int32_t core_id) {
            return (0 <= core_id && core_id < static_cast<int32_t>(cpu::NumCores));
        }

        void ExitProcess() {
            GetCurrentProcess().Exit();
            MESOSPHERE_PANIC("Process survived call to exit");
        }

        Result GetProcessId(u64 *out_process_id, ams::svc::Handle handle) {
            /* Get the object from the handle table. */
            KScopedAutoObject obj = GetCurrentProcess().GetHandleTable().GetObject<KAutoObject>(handle);
            R_UNLESS(obj.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the process from the object. */
            KProcess *process = nullptr;
            if (KProcess *p = obj->DynamicCast<KProcess *>(); p != nullptr) {
                /* The object is a process, so we can use it directly. */
                process = p;
            } else if (KThread *t = obj->DynamicCast<KThread *>(); t != nullptr) {
                /* The object is a thread, so we want to use its parent. */
                process = reinterpret_cast<KThread *>(obj.GetPointerUnsafe())->GetOwnerProcess();
            } else if (KDebug *d = obj->DynamicCast<KDebug *>(); d != nullptr) {
                /* The object is a debug, so we want to use the process it's attached to. */
                obj = d->GetProcess();

                if (obj.IsNotNull()) {
                    process = static_cast<KProcess *>(obj.GetPointerUnsafe());
                }
            }

            /* Make sure the target process exists. */
            R_UNLESS(process != nullptr, svc::ResultInvalidHandle());

            /* Get the process id. */
            *out_process_id = process->GetId();
            return ResultSuccess();
        }

        Result GetProcessList(int32_t *out_num_processes, KUserPointer<uint64_t *> out_process_ids, int32_t max_out_count) {
            /* Validate that the out count is valid. */
            R_UNLESS((0 <= max_out_count && max_out_count <= static_cast<int32_t>(std::numeric_limits<int32_t>::max() / sizeof(u64))), svc::ResultOutOfRange());

            /* Validate that the pointer is in range. */
            if (max_out_count > 0) {
                R_UNLESS(GetCurrentProcess().GetPageTable().Contains(KProcessAddress(out_process_ids.GetUnsafePointer()), max_out_count * sizeof(u64)), svc::ResultInvalidCurrentMemory());
            }

            /* Get the process list. */
            return KProcess::GetProcessList(out_num_processes, out_process_ids, max_out_count);
        }

        Result CreateProcess(ams::svc::Handle *out, const ams::svc::CreateProcessParameter &params, KUserPointer<const uint32_t *> user_caps, int32_t num_caps) {
            /* Validate the capabilities pointer. */
            R_UNLESS(num_caps >= 0, svc::ResultInvalidPointer());
            if (num_caps > 0) {
                /* Check for overflow. */
                R_UNLESS(((num_caps * sizeof(u32)) / sizeof(u32)) == static_cast<size_t>(num_caps), svc::ResultInvalidPointer());

                /* Validate that the pointer is in range. */
                R_UNLESS(GetCurrentProcess().GetPageTable().Contains(KProcessAddress(user_caps.GetUnsafePointer()), num_caps * sizeof(u32)), svc::ResultInvalidPointer());
            }

            /* Validate that the parameter flags are valid. */
            R_UNLESS((params.flags & ~ams::svc::CreateProcessFlag_All) == 0, svc::ResultInvalidEnumValue());

            /* Validate that 64-bit process is okay. */
            const bool is_64_bit = (params.flags & ams::svc::CreateProcessFlag_Is64Bit) != 0;
            if constexpr (sizeof(void *) < sizeof(u64)) {
                R_UNLESS(!is_64_bit, svc::ResultInvalidCombination());
            }

            /* Decide on an address space map region. */
            uintptr_t map_start, map_end;
            size_t map_size;
            switch (params.flags & ams::svc::CreateProcessFlag_AddressSpaceMask) {
                case ams::svc::CreateProcessFlag_AddressSpace32Bit:
                case ams::svc::CreateProcessFlag_AddressSpace32BitWithoutAlias:
                    {
                        map_start = KAddressSpaceInfo::GetAddressSpaceStart(32, KAddressSpaceInfo::Type_MapSmall);
                        map_size  = KAddressSpaceInfo::GetAddressSpaceSize(32, KAddressSpaceInfo::Type_MapSmall);
                        map_end   = map_start + map_size;
                    }
                    break;
                case ams::svc::CreateProcessFlag_AddressSpace64BitDeprecated:
                    {
                        /* 64-bit address space requires 64-bit process. */
                        R_UNLESS(is_64_bit, svc::ResultInvalidCombination());

                        map_start = KAddressSpaceInfo::GetAddressSpaceStart(36, KAddressSpaceInfo::Type_MapSmall);
                        map_size  = KAddressSpaceInfo::GetAddressSpaceSize(36, KAddressSpaceInfo::Type_MapSmall);
                        map_end   = map_start + map_size;
                    }
                    break;
                case ams::svc::CreateProcessFlag_AddressSpace64Bit:
                    {
                        /* 64-bit address space requires 64-bit process. */
                        R_UNLESS(is_64_bit, svc::ResultInvalidCombination());

                        map_start = KAddressSpaceInfo::GetAddressSpaceStart(39, KAddressSpaceInfo::Type_Map39Bit);
                        map_end   = map_start + KAddressSpaceInfo::GetAddressSpaceSize(39, KAddressSpaceInfo::Type_Map39Bit);

                        map_size  = KAddressSpaceInfo::GetAddressSpaceSize(39, KAddressSpaceInfo::Type_Heap);
                    }
                    break;
                default:
                    return svc::ResultInvalidEnumValue();
            }

            /* Validate the pool partition. */
            if (GetTargetFirmware() >= TargetFirmware_5_0_0) {
                switch (params.flags & ams::svc::CreateProcessFlag_PoolPartitionMask) {
                    case ams::svc::CreateProcessFlag_PoolPartitionApplication:
                    case ams::svc::CreateProcessFlag_PoolPartitionApplet:
                    case ams::svc::CreateProcessFlag_PoolPartitionSystem:
                    case ams::svc::CreateProcessFlag_PoolPartitionSystemNonSecure:
                        break;
                    default:
                        return svc::ResultInvalidEnumValue();
                }
            }

            /* Check that the code address is aligned. */
            R_UNLESS(util::IsAligned(params.code_address, KProcess::AslrAlignment), svc::ResultInvalidAddress());

            /* Check that the number of code pages is >= 0. */
            R_UNLESS(params.code_num_pages >= 0, svc::ResultInvalidSize());

            /* Check that the number of extra resource pages is >= 0. */
            R_UNLESS(params.system_resource_num_pages >= 0, svc::ResultInvalidSize());

            /* Convert to sizes. */
            const size_t code_num_pages            = params.code_num_pages;
            const size_t system_resource_num_pages = params.system_resource_num_pages;
            const size_t total_pages               = code_num_pages + system_resource_num_pages;
            const size_t code_size                 = code_num_pages * PageSize;
            const size_t system_resource_size      = system_resource_num_pages * PageSize;
            const size_t total_size                = code_size + system_resource_size;

            /* Check for overflow. */
            R_UNLESS((code_size / PageSize) == code_num_pages,                       svc::ResultInvalidSize());
            R_UNLESS((system_resource_size / PageSize) == system_resource_num_pages, svc::ResultInvalidSize());
            R_UNLESS((code_num_pages + system_resource_num_pages) >= code_num_pages, svc::ResultOutOfMemory());
            R_UNLESS((total_size / PageSize) == total_pages,                         svc::ResultInvalidSize());

            /* Check that the number of pages is valid. */
            R_UNLESS(code_num_pages < (map_size / PageSize), svc::ResultInvalidMemoryRegion());

            /* Validate that the code falls within the map reigon. */
            R_UNLESS(map_start <= params.code_address,                      svc::ResultInvalidMemoryRegion());
            R_UNLESS(params.code_address < params.code_address + code_size, svc::ResultInvalidMemoryRegion());
            R_UNLESS(params.code_address + code_size - 1 <= map_end - 1,    svc::ResultInvalidMemoryRegion());

            /* Check that the number of pages is valid for the kernel address space. */
            R_UNLESS(code_num_pages            < (kern::MainMemorySize / PageSize), svc::ResultOutOfMemory());
            R_UNLESS(system_resource_num_pages < (kern::MainMemorySize / PageSize), svc::ResultOutOfMemory());
            R_UNLESS(total_pages               < (kern::MainMemorySize / PageSize), svc::ResultOutOfMemory());

            /* Check that optimized memory allocation is used only for applications. */
            const bool optimize_allocs = (params.flags & ams::svc::CreateProcessFlag_OptimizeMemoryAllocation) != 0;
            const bool is_application  = (params.flags & ams::svc::CreateProcessFlag_IsApplication) != 0;
            R_UNLESS(!optimize_allocs || is_application, svc::ResultBusy());

            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Create the new process. */
            KProcess *process = KProcess::Create();
            R_UNLESS(process != nullptr, svc::ResultOutOfResource());

            /* Ensure that the only reference to the process is in the handle table when we're done. */
            ON_SCOPE_EXIT { process->Close(); };

            /* Get the resource limit from the handle. */
            KScopedAutoObject resource_limit = handle_table.GetObject<KResourceLimit>(params.reslimit);
            R_UNLESS(resource_limit.IsNotNull() || params.reslimit == ams::svc::InvalidHandle, svc::ResultInvalidHandle());

            /* Decide on a resource limit for the process. */
            KResourceLimit *process_resource_limit = resource_limit.IsNotNull() ? resource_limit.GetPointerUnsafe() : std::addressof(Kernel::GetSystemResourceLimit());

            /* Get the pool for the process. */
            const auto pool = [] ALWAYS_INLINE_LAMBDA (u32 flags) -> KMemoryManager::Pool {
                if (GetTargetFirmware() >= TargetFirmware_5_0_0) {
                    switch (flags & ams::svc::CreateProcessFlag_PoolPartitionMask) {
                        case ams::svc::CreateProcessFlag_PoolPartitionApplication:
                            return KMemoryManager::Pool_Application;
                        case ams::svc::CreateProcessFlag_PoolPartitionApplet:
                            return KMemoryManager::Pool_Applet;
                        case ams::svc::CreateProcessFlag_PoolPartitionSystem:
                            return KMemoryManager::Pool_System;
                        case ams::svc::CreateProcessFlag_PoolPartitionSystemNonSecure:
                        default:
                            return KMemoryManager::Pool_SystemNonSecure;
                    }
                } else if (GetTargetFirmware() >= TargetFirmware_4_0_0) {
                    if ((flags & ams::svc::CreateProcessFlag_DeprecatedUseSecureMemory) != 0) {
                        return KMemoryManager::Pool_Secure;
                    } else {
                        return static_cast<KMemoryManager::Pool>(KSystemControl::GetCreateProcessMemoryPool());
                    }
                } else {
                    return static_cast<KMemoryManager::Pool>(KSystemControl::GetCreateProcessMemoryPool());
                }
            }(params.flags);

            /* Initialize the process. */
            R_TRY(process->Initialize(params, user_caps, num_caps, process_resource_limit, pool));

            /* Register the process. */
            KProcess::Register(process);

            /* Add the process to the handle table. */
            R_TRY(handle_table.Add(out, process));

            return ResultSuccess();
        }

        template<typename T>
        Result CreateProcess(ams::svc::Handle *out, KUserPointer<const T *> user_parameters, KUserPointer<const uint32_t *> user_caps, int32_t num_caps) {
            /* Read the parameters from user space. */
            T params;
            R_TRY(user_parameters.CopyTo(std::addressof(params)));

            /* Invoke the implementation. */
            if constexpr (std::same_as<T, ams::svc::CreateProcessParameter>) {
                return CreateProcess(out, params, user_caps, num_caps);
            } else {
                /* Convert the parameters. */
                ams::svc::CreateProcessParameter converted_params;
                static_assert(sizeof(T{}.name) == sizeof(ams::svc::CreateProcessParameter{}.name));

                std::memcpy(converted_params.name, params.name, sizeof(converted_params.name));
                converted_params.version                   = params.version;
                converted_params.program_id                = params.program_id;
                converted_params.code_address              = params.code_address;
                converted_params.code_num_pages            = params.code_num_pages;
                converted_params.flags                     = params.flags;
                converted_params.reslimit                  = params.reslimit;
                converted_params.system_resource_num_pages = params.system_resource_num_pages;

                /* Invoke. */
                return CreateProcess(out, converted_params, user_caps, num_caps);
            }
        }

        Result StartProcess(ams::svc::Handle process_handle, int32_t priority, int32_t core_id, uint64_t main_thread_stack_size) {
            /* Validate stack size. */
            R_UNLESS(main_thread_stack_size == static_cast<size_t>(main_thread_stack_size), svc::ResultOutOfMemory());

            /* Get the target process. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Validate the core id. */
            R_UNLESS(IsValidCoreId(core_id),                           svc::ResultInvalidCoreId());
            R_UNLESS(((1ul << core_id) & process->GetCoreMask()) != 0, svc::ResultInvalidCoreId());

            /* Validate the priority. */
            R_UNLESS(ams::svc::HighestThreadPriority <= priority && priority <= ams::svc::LowestThreadPriority, svc::ResultInvalidPriority());
            R_UNLESS(process->CheckThreadPriority(priority),                                                    svc::ResultInvalidPriority());

            /* Set the process's ideal processor. */
            process->SetIdealCoreId(core_id);

            /* Run the process. */
            R_TRY(process->Run(priority, static_cast<size_t>(main_thread_stack_size)));

            /* Open a reference to the process, since it's now running. */
            process->Open();

            return ResultSuccess();
        }

        Result TerminateProcess(ams::svc::Handle process_handle) {
            /* Get the target process. */
            KProcess *process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle).ReleasePointerUnsafe();
            R_UNLESS(process != nullptr, svc::ResultInvalidHandle());

            if (process != GetCurrentProcessPointer()) {
                /* We're terminating another process. Close our reference after terminating the process. */
                ON_SCOPE_EXIT { process->Close(); };

                /* Terminate the process. */
                R_TRY(process->Terminate());
            } else {
                /* We're terminating ourselves. Close our reference immediately. */
                process->Close();

                /* Exit. */
                ExitProcess();
            }

            return ResultSuccess();
        }

        Result GetProcessInfo(int64_t *out, ams::svc::Handle process_handle, ams::svc::ProcessInfoType info_type) {
            /* Get the target process. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the info. */
            switch (info_type) {
                case ams::svc::ProcessInfoType_ProcessState:
                    {
                        /* Get the process's state. */
                        KProcess::State state;
                        {
                            KScopedLightLock proc_lk(process->GetStateLock());
                            KScopedSchedulerLock sl;

                            state = process->GetState();
                        }

                        /* Convert to svc state. */
                        switch (state) {
                            case KProcess::State_Created:         *out = ams::svc::ProcessState_Created;         break;
                            case KProcess::State_CreatedAttached: *out = ams::svc::ProcessState_CreatedAttached; break;
                            case KProcess::State_Running:         *out = ams::svc::ProcessState_Running;         break;
                            case KProcess::State_Crashed:         *out = ams::svc::ProcessState_Crashed;         break;
                            case KProcess::State_RunningAttached: *out = ams::svc::ProcessState_RunningAttached; break;
                            case KProcess::State_Terminating:     *out = ams::svc::ProcessState_Terminating;     break;
                            case KProcess::State_Terminated:      *out = ams::svc::ProcessState_Terminated;      break;
                            case KProcess::State_DebugBreak:      *out = ams::svc::ProcessState_DebugBreak;      break;
                            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                        }
                    }
                    break;
                default:
                    return svc::ResultInvalidEnumValue();
            }

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    void ExitProcess64() {
        return ExitProcess();
    }

    Result GetProcessId64(uint64_t *out_process_id, ams::svc::Handle process_handle) {
        return GetProcessId(out_process_id, process_handle);
    }

    Result GetProcessList64(int32_t *out_num_processes, KUserPointer<uint64_t *> out_process_ids, int32_t max_out_count) {
        return GetProcessList(out_num_processes, out_process_ids, max_out_count);
    }

    Result CreateProcess64(ams::svc::Handle *out_handle, KUserPointer<const ams::svc::lp64::CreateProcessParameter *> parameters, KUserPointer<const uint32_t *> caps, int32_t num_caps) {
        return CreateProcess(out_handle, parameters, caps, num_caps);
    }

    Result StartProcess64(ams::svc::Handle process_handle, int32_t priority, int32_t core_id, uint64_t main_thread_stack_size) {
        return StartProcess(process_handle, priority, core_id, main_thread_stack_size);
    }

    Result TerminateProcess64(ams::svc::Handle process_handle) {
        return TerminateProcess(process_handle);
    }

    Result GetProcessInfo64(int64_t *out_info, ams::svc::Handle process_handle, ams::svc::ProcessInfoType info_type) {
        return GetProcessInfo(out_info, process_handle, info_type);
    }

    /* ============================= 64From32 ABI ============================= */

    void ExitProcess64From32() {
        return ExitProcess();
    }

    Result GetProcessId64From32(uint64_t *out_process_id, ams::svc::Handle process_handle) {
        return GetProcessId(out_process_id, process_handle);
    }

    Result GetProcessList64From32(int32_t *out_num_processes, KUserPointer<uint64_t *> out_process_ids, int32_t max_out_count) {
        return GetProcessList(out_num_processes, out_process_ids, max_out_count);
    }

    Result CreateProcess64From32(ams::svc::Handle *out_handle, KUserPointer<const ams::svc::ilp32::CreateProcessParameter *> parameters, KUserPointer<const uint32_t *> caps, int32_t num_caps) {
        return CreateProcess(out_handle, parameters, caps, num_caps);
    }

    Result StartProcess64From32(ams::svc::Handle process_handle, int32_t priority, int32_t core_id, uint64_t main_thread_stack_size) {
        return StartProcess(process_handle, priority, core_id, main_thread_stack_size);
    }

    Result TerminateProcess64From32(ams::svc::Handle process_handle) {
        return TerminateProcess(process_handle);
    }

    Result GetProcessInfo64From32(int64_t *out_info, ams::svc::Handle process_handle, ams::svc::ProcessInfoType info_type) {
        return GetProcessInfo(out_info, process_handle, info_type);
    }

}
