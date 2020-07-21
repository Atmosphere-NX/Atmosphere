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

        Result GetInfo(u64 *out, ams::svc::InfoType info_type, ams::svc::Handle handle, u64 info_subtype) {
            MESOSPHERE_LOG("GetInfo(%p, %u, %08x, %lu) was called\n", out, static_cast<u32>(info_type), static_cast<u32>(handle), info_subtype);
            ON_SCOPE_EXIT{ MESOSPHERE_LOG("GetInfo returned %016lx\n", *out); };

            switch (info_type) {
                case ams::svc::InfoType_CoreMask:
                case ams::svc::InfoType_PriorityMask:
                case ams::svc::InfoType_AliasRegionAddress:
                case ams::svc::InfoType_AliasRegionSize:
                case ams::svc::InfoType_HeapRegionAddress:
                case ams::svc::InfoType_HeapRegionSize:
                case ams::svc::InfoType_TotalMemorySize:
                case ams::svc::InfoType_UsedMemorySize:
                case ams::svc::InfoType_AslrRegionAddress:
                case ams::svc::InfoType_AslrRegionSize:
                case ams::svc::InfoType_StackRegionAddress:
                case ams::svc::InfoType_StackRegionSize:
                case ams::svc::InfoType_ProgramId:
                case ams::svc::InfoType_InitialProcessIdRange:
                case ams::svc::InfoType_UserExceptionContextAddress:
                case ams::svc::InfoType_TotalNonSystemMemorySize:
                case ams::svc::InfoType_UsedNonSystemMemorySize:
                    {
                        /* These info types don't support non-zero subtypes. */
                        R_UNLESS(info_subtype == 0,  svc::ResultInvalidCombination());

                        /* Get the process from its handle. */
                        KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(handle);
                        R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

                        switch (info_type) {
                            case ams::svc::InfoType_CoreMask:
                                *out = process->GetCoreMask();
                                break;
                            case ams::svc::InfoType_PriorityMask:
                                *out = process->GetPriorityMask();
                                break;
                            case ams::svc::InfoType_AliasRegionAddress:
                                *out = GetInteger(process->GetPageTable().GetAliasRegionStart());
                                break;
                            case ams::svc::InfoType_AliasRegionSize:
                                *out = process->GetPageTable().GetAliasRegionSize();
                                break;
                            case ams::svc::InfoType_HeapRegionAddress:
                                *out = GetInteger(process->GetPageTable().GetHeapRegionStart());
                                break;
                            case ams::svc::InfoType_HeapRegionSize:
                                *out = process->GetPageTable().GetHeapRegionSize();
                                break;
                            case ams::svc::InfoType_TotalMemorySize:
                                *out = process->GetTotalUserPhysicalMemorySize();
                                break;
                            case ams::svc::InfoType_UsedMemorySize:
                                *out = process->GetUsedUserPhysicalMemorySize();
                                break;
                            case ams::svc::InfoType_AslrRegionAddress:
                                *out = GetInteger(process->GetPageTable().GetAliasCodeRegionStart());
                                break;
                            case ams::svc::InfoType_AslrRegionSize:
                                *out = process->GetPageTable().GetAliasCodeRegionSize();
                                break;
                            case ams::svc::InfoType_StackRegionAddress:
                                *out = GetInteger(process->GetPageTable().GetStackRegionStart());
                                break;
                            case ams::svc::InfoType_StackRegionSize:
                                *out = process->GetPageTable().GetStackRegionSize();
                                break;
                            case ams::svc::InfoType_ProgramId:
                                *out = process->GetProgramId();
                                break;
                            case ams::svc::InfoType_InitialProcessIdRange:
                                /* TODO: Detect exactly 4.0.0 target firmware, do the right thing. */
                                return svc::ResultInvalidEnumValue();
                            case ams::svc::InfoType_UserExceptionContextAddress:
                                *out = GetInteger(process->GetProcessLocalRegionAddress());
                                break;
                            case ams::svc::InfoType_TotalNonSystemMemorySize:
                                *out = process->GetTotalNonSystemUserPhysicalMemorySize();
                                break;
                            case ams::svc::InfoType_UsedNonSystemMemorySize:
                                *out = process->GetUsedNonSystemUserPhysicalMemorySize();
                                break;
                            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                        }
                    }
                    break;
                case ams::svc::InfoType_ResourceLimit:
                    {
                        /* Verify the input handle is invalid. */
                        R_UNLESS(handle == ams::svc::InvalidHandle, svc::ResultInvalidHandle());

                        /* Verify the sub-type is valid. */
                        R_UNLESS(info_subtype == 0, svc::ResultInvalidCombination());

                        /* Get the handle table and resource limit. */
                        KHandleTable &handle_table     = GetCurrentProcess().GetHandleTable();
                        KResourceLimit *resource_limit = GetCurrentProcess().GetResourceLimit();

                        if (resource_limit != nullptr) {
                            /* Get a new handle for the resource limit. */
                            ams::svc::Handle tmp;
                            R_TRY(handle_table.Add(std::addressof(tmp), resource_limit));

                            /* Set the output. */
                            *out = tmp;
                        } else {
                            /* Set the output. */
                            *out = ams::svc::InvalidHandle;
                        }
                    }
                    break;
                case ams::svc::InfoType_RandomEntropy:
                    {
                        /* Verify the input handle is invalid. */
                        R_UNLESS(handle == ams::svc::InvalidHandle, svc::ResultInvalidHandle());

                        /* Verify the requested entropy is valid. */
                        R_UNLESS(info_subtype < 4, svc::ResultInvalidCombination());

                        /* Get the entropy. */
                        *out = GetCurrentProcess().GetRandomEntropy(info_subtype);
                    }
                    break;
                default:
                    return svc::ResultInvalidEnumValue();
            }

            return ResultSuccess();
        }

        constexpr bool IsValidMemoryPool(u64 pool) {
            switch (static_cast<KMemoryManager::Pool>(pool)) {
                case KMemoryManager::Pool_Application:
                case KMemoryManager::Pool_Applet:
                case KMemoryManager::Pool_System:
                case KMemoryManager::Pool_SystemNonSecure:
                    return true;
                default:
                    return false;
            }
        }

        Result GetSystemInfo(u64 *out, ams::svc::SystemInfoType info_type, ams::svc::Handle handle, u64 info_subtype) {
            MESOSPHERE_LOG("GetSystemInfo(%p, %u, %08x, %lu) was called\n", out, static_cast<u32>(info_type), static_cast<u32>(handle), info_subtype);
            ON_SCOPE_EXIT{ MESOSPHERE_LOG("GetSystemInfo returned %016lx\n", *out); };

            switch (info_type) {
                case ams::svc::SystemInfoType_TotalPhysicalMemorySize:
                case ams::svc::SystemInfoType_UsedPhysicalMemorySize:
                    {
                        /* Verify the input handle is invalid. */
                        R_UNLESS(handle == ams::svc::InvalidHandle, svc::ResultInvalidHandle());

                        /* Verify the sub-type is valid. */
                        R_UNLESS(IsValidMemoryPool(info_subtype), svc::ResultInvalidCombination());

                        /* Convert to pool. */
                        const auto pool = static_cast<KMemoryManager::Pool>(info_subtype);

                        /* Get the memory size. */
                        auto &mm = Kernel::GetMemoryManager();
                        switch (info_type) {
                            case ams::svc::SystemInfoType_TotalPhysicalMemorySize:
                                *out = mm.GetSize(pool);
                                break;
                            case ams::svc::SystemInfoType_UsedPhysicalMemorySize:
                                *out = mm.GetSize(pool) - mm.GetFreeSize(pool);
                                break;
                            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                        }
                    }
                    break;
                case ams::svc::SystemInfoType_InitialProcessIdRange:
                    {
                        R_UNLESS(handle == ams::svc::InvalidHandle, svc::ResultInvalidHandle());
                        switch (static_cast<ams::svc::InitialProcessIdRangeInfo>(info_subtype)) {
                            case ams::svc::InitialProcessIdRangeInfo_Minimum:
                                MESOSPHERE_ABORT_UNLESS(GetInitialProcessIdMin() <= GetInitialProcessIdMax());
                                *out = GetInitialProcessIdMin();
                                break;
                            case ams::svc::InitialProcessIdRangeInfo_Maximum:
                                MESOSPHERE_ABORT_UNLESS(GetInitialProcessIdMin() <= GetInitialProcessIdMax());
                                *out = GetInitialProcessIdMax();
                                break;
                            default:
                                return svc::ResultInvalidCombination();
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

    Result GetInfo64(uint64_t *out, ams::svc::InfoType info_type, ams::svc::Handle handle, uint64_t info_subtype) {
        return GetInfo(out, info_type, handle, info_subtype);
    }

    Result GetSystemInfo64(uint64_t *out, ams::svc::SystemInfoType info_type, ams::svc::Handle handle, uint64_t info_subtype) {
        return GetSystemInfo(out, info_type, handle, info_subtype);
    }

    /* ============================= 64From32 ABI ============================= */

    Result GetInfo64From32(uint64_t *out, ams::svc::InfoType info_type, ams::svc::Handle handle, uint64_t info_subtype) {
        return GetInfo(out, info_type, handle, info_subtype);
    }

    Result GetSystemInfo64From32(uint64_t *out, ams::svc::SystemInfoType info_type, ams::svc::Handle handle, uint64_t info_subtype) {
        return GetSystemInfo(out, info_type, handle, info_subtype);
    }

}
