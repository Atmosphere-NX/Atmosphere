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
                case ams::svc::InfoType_AliasRegionAddress:
                case ams::svc::InfoType_AliasRegionSize:
                case ams::svc::InfoType_HeapRegionAddress:
                case ams::svc::InfoType_HeapRegionSize:
                case ams::svc::InfoType_AslrRegionAddress:
                case ams::svc::InfoType_AslrRegionSize:
                case ams::svc::InfoType_StackRegionAddress:
                case ams::svc::InfoType_StackRegionSize:
                    {
                        /* These info types don't support non-zero subtypes. */
                        R_UNLESS(info_subtype == 0,  svc::ResultInvalidCombination());

                        /* Get the process from its handle. */
                        KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(handle);
                        R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

                        switch (info_type) {
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
                            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                        }
                    }
                    break;
                default:
                    return svc::ResultInvalidEnumValue();
            }

            return ResultSuccess();
        }

        Result GetSystemInfo(u64 *out, ams::svc::SystemInfoType info_type, ams::svc::Handle handle, u64 info_subtype) {
            MESOSPHERE_LOG("GetSystemInfo(%p, %u, %08x, %lu) was called\n", out, static_cast<u32>(info_type), static_cast<u32>(handle), info_subtype);
            ON_SCOPE_EXIT{ MESOSPHERE_LOG("GetSystemInfo returned %016lx\n", *out); };

            switch (info_type) {
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
