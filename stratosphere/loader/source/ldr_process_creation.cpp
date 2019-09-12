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

#include <limits>
#include <stratosphere/map.hpp>
#include <stratosphere/rnd.hpp>
#include <stratosphere/util.hpp>

#include "ldr_capabilities.hpp"
#include "ldr_content_management.hpp"
#include "ldr_ecs.hpp"
#include "ldr_launch_record.hpp"
#include "ldr_meta.hpp"
#include "ldr_patcher.hpp"
#include "ldr_process_creation.hpp"
#include "ldr_ro_manager.hpp"

namespace sts::ldr {

    namespace {

        /* Convenience defines. */
        constexpr size_t BaseAddressAlignment = 0x200000;
        constexpr size_t SystemResourceSizeAlignment = 0x200000;
        constexpr size_t SystemResourceSizeMax = 0x1FE00000;

        /* Types. */
        enum NsoIndex {
            Nso_Rtld    =  0,
            Nso_Main    =  1,
            Nso_SubSdk0 =  2,
            Nso_SubSdk1 =  3,
            Nso_SubSdk2 =  4,
            Nso_SubSdk3 =  5,
            Nso_SubSdk4 =  6,
            Nso_SubSdk5 =  7,
            Nso_SubSdk6 =  8,
            Nso_SubSdk7 =  9,
            Nso_SubSdk8 = 10,
            Nso_SubSdk9 = 11,
            Nso_Sdk     = 12,
            Nso_Count,
        };

        constexpr const char *GetNsoName(size_t idx) {
            if (idx >= Nso_Count) {
                std::abort();
            }

            constexpr const char *NsoNames[Nso_Count] = {
                "rtld",
                "main",
                "subsdk0",
                "subsdk1",
                "subsdk2",
                "subsdk3",
                "subsdk4",
                "subsdk5",
                "subsdk6",
                "subsdk7",
                "subsdk8",
                "subsdk9",
                "sdk",
            };
            return NsoNames[idx];
        }

        struct CreateProcessInfo {
            char name[12];
            u32  version;
            ncm::TitleId title_id;
            u64  code_address;
            u32  code_num_pages;
            u32  flags;
            Handle reslimit;
            u32  system_resource_num_pages;
        };
        static_assert(sizeof(CreateProcessInfo) == 0x30, "CreateProcessInfo definition!");

        struct ProcessInfo {
            AutoHandle process_handle;
            uintptr_t args_address;
            size_t    args_size;
            uintptr_t nso_address[Nso_Count];
            size_t    nso_size[Nso_Count];
        };

        /* Global NSO header cache. */
        bool g_has_nso[Nso_Count];
        NsoHeader g_nso_headers[Nso_Count];

        /* Anti-downgrade. */
        struct MinimumTitleVersion {
            ncm::TitleId title_id;
            u32 version;
        };

        constexpr u32 MakeSystemVersion(u32 major, u32 minor, u32 micro) {
            return (major << 26) | (minor << 20) | (micro << 16);
        }

        constexpr MinimumTitleVersion g_MinimumTitleVersions810[] = {
            {ncm::TitleId::Settings,    1},
            {ncm::TitleId::Bus,         1},
            {ncm::TitleId::Audio,       1},
            {ncm::TitleId::NvServices,  1},
            {ncm::TitleId::Ns,          1},
            {ncm::TitleId::Ssl,         1},
            {ncm::TitleId::Es,          1},
            {ncm::TitleId::Creport,     1},
            {ncm::TitleId::Ro,          1},
        };
        constexpr size_t g_MinimumTitleVersionsCount810 = util::size(g_MinimumTitleVersions810);

        constexpr MinimumTitleVersion g_MinimumTitleVersions900[] = {
            /* All non-Development System Modules. */
            {ncm::TitleId::Usb,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Tma,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Boot2,       MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Settings,    MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Bus,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Bluetooth,   MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Bcat,        MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Dmnt,        MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::Friends,     MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Nifm,        MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Ptm,         MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Shell,       MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::BsdSockets,  MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Hid,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Audio,       MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::LogManager,  MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Wlan,        MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Cs,          MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::Ldn,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::NvServices,  MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Pcv,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Ppc,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::NvnFlinger,  MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Pcie,        MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Account,     MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Ns,          MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Nfc,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Psc,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::CapSrv,      MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Am,          MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Ssl,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Nim,         MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Cec,         MakeSystemVersion(9, 0, 0)}, */
         /* {ncm::TitleId::Tspm,        MakeSystemVersion(9, 0, 0)}, */
         /* {ncm::TitleId::Spl,         MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::Lbl,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Btm,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Erpt,        MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Time,        MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::Vi,          MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Pctl,        MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Npns,        MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Eupld,       MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Glue,        MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Eclct,       MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Es,          MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Fatal,       MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Grc,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Creport,     MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Ro,          MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Profiler,    MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::Sdb,         MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Migration,   MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Jit,         MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::JpegDec,     MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::SafeMode,    MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::Olsc,        MakeSystemVersion(9, 0, 0)},
         /* {ncm::TitleId::Dt,          MakeSystemVersion(9, 0, 0)}, */
         /* {ncm::TitleId::Nd,          MakeSystemVersion(9, 0, 0)}, */
            {ncm::TitleId::Ngct,        MakeSystemVersion(9, 0, 0)},

            /* All Web Applets. */
            {ncm::TitleId::AppletWeb,           MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::AppletShop,          MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::AppletOfflineWeb,    MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::AppletLoginShare,    MakeSystemVersion(9, 0, 0)},
            {ncm::TitleId::AppletWifiWebAuth,   MakeSystemVersion(9, 0, 0)},
        };
        constexpr size_t g_MinimumTitleVersionsCount900 = util::size(g_MinimumTitleVersions900);

        Result ValidateTitleVersion(ncm::TitleId title_id, u32 version) {
            if (GetRuntimeFirmwareVersion() < FirmwareVersion_810) {
                return ResultSuccess;
            } else {
#ifdef LDR_VALIDATE_PROCESS_VERSION
                const MinimumTitleVersion *entries = nullptr;
                size_t num_entries = 0;
                switch (GetRuntimeFirmwareVersion()) {
                    case FirmwareVersion_810:
                        entries = g_MinimumTitleVersions810;
                        num_entries = g_MinimumTitleVersionsCount810;
                        break;
                    case FirmwareVersion_900:
                        entries = g_MinimumTitleVersions900;
                        num_entries = g_MinimumTitleVersionsCount900;
                        break;
                    default:
                        entries = nullptr;
                        num_entries = 0;
                        break;
                }

                for (size_t i = 0; i < num_entries; i++) {
                    if (entries[i].title_id == title_id && entries[i].version > version) {
                        return ResultLoaderInvalidVersion;
                    }
                }

                return ResultSuccess;
#else
                return ResultSuccess;
#endif
            }
        }

        /* Helpers. */
        Result GetProgramInfoFromMeta(ProgramInfo *out, const Meta *meta) {
            /* Copy basic info. */
            out->main_thread_priority = meta->npdm->main_thread_priority;
            out->default_cpu_id = meta->npdm->default_cpu_id;
            out->main_thread_stack_size = meta->npdm->main_thread_stack_size;
            out->title_id = meta->aci->title_id;

            /* Copy access controls. */
            size_t offset = 0;
#define COPY_ACCESS_CONTROL(source, which) \
            ({ \
                const size_t size = meta->source->which##_size; \
                if (offset + size > sizeof(out->ac_buffer)) { \
                    return ResultLoaderInternalError; \
                } \
                out->source##_##which##_size = size; \
                std::memcpy(out->ac_buffer + offset, meta->source##_##which, size); \
                offset += size; \
            })

            /* Copy all access controls to buffer. */
            COPY_ACCESS_CONTROL(acid, sac);
            COPY_ACCESS_CONTROL(aci, sac);
            COPY_ACCESS_CONTROL(acid, fac);
            COPY_ACCESS_CONTROL(aci, fah);
#undef COPY_ACCESS_CONTROL

            /* Copy flags. */
            out->flags = caps::GetProgramInfoFlags(meta->acid_kac, meta->acid->kac_size);
            return ResultSuccess;
        }

        bool IsApplet(const Meta *meta) {
            return (caps::GetProgramInfoFlags(meta->aci_kac, meta->aci->kac_size) & ProgramInfoFlag_ApplicationTypeMask) == ProgramInfoFlag_Applet;
        }

        bool IsApplication(const Meta *meta) {
            return (caps::GetProgramInfoFlags(meta->aci_kac, meta->aci->kac_size) & ProgramInfoFlag_ApplicationTypeMask) == ProgramInfoFlag_Application;
        }

        Npdm::AddressSpaceType GetAddressSpaceType(const Meta *meta) {
            return static_cast<Npdm::AddressSpaceType>((meta->npdm->flags & Npdm::MetaFlag_AddressSpaceTypeMask) >> Npdm::MetaFlag_AddressSpaceTypeShift);
        }

        Acid::PoolPartition GetPoolPartition(const Meta *meta) {
            return static_cast<Acid::PoolPartition>((meta->acid->flags & Acid::AcidFlag_PoolPartitionMask) >> Acid::AcidFlag_PoolPartitionShift);
        }

        Result LoadNsoHeaders(ncm::TitleId title_id, NsoHeader *nso_headers, bool *has_nso) {
            /* Clear NSOs. */
            std::memset(nso_headers, 0, sizeof(*nso_headers) * Nso_Count);
            std::memset(has_nso, 0, sizeof(*has_nso) * Nso_Count);

            for (size_t i = 0; i < Nso_Count; i++) {
                FILE *f = nullptr;
                if (R_SUCCEEDED(OpenCodeFile(f, title_id, GetNsoName(i)))) {
                    ON_SCOPE_EXIT { fclose(f); };
                    /* Read NSO header. */
                    if (fread(nso_headers + i, sizeof(*nso_headers), 1, f) != 1) {
                        return ResultLoaderInvalidNso;
                    }
                    has_nso[i] = true;
                }
            }

            return ResultSuccess;
        }

        Result ValidateNsoHeaders(const NsoHeader *nso_headers, const bool *has_nso) {
            /* We must always have a main. */
            if (!has_nso[Nso_Main]) {
                return ResultLoaderInvalidNso;
            }

            /* If we don't have an RTLD, we must only have a main. */
            if (!has_nso[Nso_Rtld]) {
                for (size_t i = Nso_Main + 1; i < Nso_Count; i++) {
                    if (has_nso[i]) {
                        return ResultLoaderInvalidNso;
                    }
                }
            }

            /* All NSOs must have zero text offset. */
            for (size_t i = 0; i < Nso_Count; i++) {
                if (nso_headers[i].text_dst_offset != 0) {
                    return ResultLoaderInvalidNso;
                }
            }

            return ResultSuccess;
        }

        Result ValidateMeta(const Meta *meta, const ncm::TitleLocation &loc) {
            /* Validate version. */
            R_TRY(ValidateTitleVersion(loc.title_id, meta->npdm->version));

            /* Validate title id. */
            if (meta->aci->title_id < meta->acid->title_id_min || meta->aci->title_id > meta->acid->title_id_max) {
                return ResultLoaderInvalidProgramId;
            }

            /* Validate the kernel capabilities. */
            R_TRY(caps::ValidateCapabilities(meta->acid_kac, meta->acid->kac_size, meta->aci_kac, meta->aci->kac_size));

            /* All good. */
            return ResultSuccess;
        }

        Result GetCreateProcessFlags(u32 *out, const Meta *meta, const u32 ldr_flags) {
            const u8 meta_flags = meta->npdm->flags;

            u32 flags = 0;

            /* Set Is64Bit. */
            if (meta_flags & Npdm::MetaFlag_Is64Bit) {
                flags |= svc::CreateProcessFlag_Is64Bit;
            }

            /* Set AddressSpaceType. */
            switch (GetAddressSpaceType(meta)) {
                case Npdm::AddressSpaceType_32Bit:
                    flags |= svc::CreateProcessFlag_AddressSpace32Bit;
                    break;
                case Npdm::AddressSpaceType_64BitDeprecated:
                    flags |= svc::CreateProcessFlag_AddressSpace64BitDeprecated;
                    break;
                case Npdm::AddressSpaceType_32BitWithoutAlias:
                    flags |= svc::CreateProcessFlag_AddressSpace32BitWithoutAlias;
                    break;
                case Npdm::AddressSpaceType_64Bit:
                    flags |= svc::CreateProcessFlag_AddressSpace64Bit;
                    break;
                default:
                    return ResultLoaderInvalidMeta;
            }

            /* Set Enable Debug. */
            if (ldr_flags & CreateProcessFlag_EnableDebug) {
                flags |= svc::CreateProcessFlag_EnableDebug;
            }

            /* Set Enable ASLR. */
            if (!(ldr_flags & CreateProcessFlag_DisableAslr)) {
                flags |= svc::CreateProcessFlag_EnableAslr;
            }

            /* Set Is Application. */
            if (IsApplication(meta)) {
                flags |= svc::CreateProcessFlag_IsApplication;

                /* 7.0.0+: Set OptimizeMemoryAllocation if relevant. */
                if (GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
                    if (meta_flags & Npdm::MetaFlag_OptimizeMemoryAllocation) {
                        flags |= svc::CreateProcessFlag_OptimizeMemoryAllocation;
                    }
                }
            }

            /* 5.0.0+ Set Pool Partition. */
            if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
                switch (GetPoolPartition(meta)) {
                    case Acid::PoolPartition_Application:
                        if (IsApplet(meta)) {
                            flags |= svc::CreateProcessFlag_PoolPartitionApplet;
                        } else {
                            flags |= svc::CreateProcessFlag_PoolPartitionApplication;
                        }
                        break;
                    case Acid::PoolPartition_Applet:
                        flags |= svc::CreateProcessFlag_PoolPartitionApplet;
                        break;
                    case Acid::PoolPartition_System:
                        flags |= svc::CreateProcessFlag_PoolPartitionSystem;
                        break;
                    case Acid::PoolPartition_SystemNonSecure:
                        flags |= svc::CreateProcessFlag_PoolPartitionSystemNonSecure;
                        break;
                    default:
                        return ResultLoaderInvalidMeta;
                }
            } else if (GetRuntimeFirmwareVersion() >= FirmwareVersion_400) {
                /* On 4.0.0+, the corresponding bit was simply "UseSecureMemory". */
                if (meta->acid->flags & Acid::AcidFlag_DeprecatedUseSecureMemory) {
                    flags |= svc::CreateProcessFlag_DeprecatedUseSecureMemory;
                }
            }

            *out = flags;
            return ResultSuccess;
        }

        Result GetCreateProcessInfo(CreateProcessInfo *out, const Meta *meta, u32 flags, Handle reslimit_h) {
            /* Clear output. */
            std::memset(out, 0, sizeof(*out));

            /* Set name, version, title id, resource limit handle. */
            std::memcpy(out->name, meta->npdm->title_name, sizeof(out->name) - 1);
            out->version = meta->npdm->version;
            out->title_id = meta->aci->title_id;
            out->reslimit = reslimit_h;

            /* Set flags. */
            R_TRY(GetCreateProcessFlags(&out->flags, meta, flags));

            /* 3.0.0+ System Resource Size. */
            if (GetRuntimeFirmwareVersion() >= FirmwareVersion_300) {
                /* Validate size is aligned. */
                if (meta->npdm->system_resource_size & (SystemResourceSizeAlignment - 1)) {
                    return ResultLoaderInvalidSize;
                }
                /* Validate system resource usage. */
                if (meta->npdm->system_resource_size) {
                    /* Process must be 64-bit. */
                    if (!(out->flags & svc::CreateProcessFlag_AddressSpace64Bit)) {
                        return ResultLoaderInvalidMeta;
                    }

                    /* Process must be application or applet. */
                    if (!IsApplication(meta) && !IsApplet(meta)) {
                        return ResultLoaderInvalidMeta;
                    }

                    /* Size must be less than or equal to max. */
                    if (meta->npdm->system_resource_size > SystemResourceSizeMax) {
                        return ResultLoaderInvalidMeta;
                    }
                }
                out->system_resource_num_pages = meta->npdm->system_resource_size >> 12;
            }

            return ResultSuccess;
        }

        Result DecideAddressSpaceLayout(ProcessInfo *out, CreateProcessInfo *out_cpi, const NsoHeader *nso_headers, const bool *has_nso, const args::ArgumentInfo *arg_info) {
            /* Clear output. */
            out->args_address = 0;
            out->args_size = 0;
            std::memset(out->nso_address, 0, sizeof(out->nso_address));
            std::memset(out->nso_size, 0, sizeof(out->nso_size));

            size_t total_size = 0;

            /* Calculate base offsets. */
            for (size_t i = 0; i < Nso_Count; i++) {
                if (has_nso[i]) {
                    out->nso_address[i] = total_size;
                    const size_t text_end = nso_headers[i].text_dst_offset + nso_headers[i].text_size;
                    const size_t ro_end   = nso_headers[i].ro_dst_offset   + nso_headers[i].ro_size;
                    const size_t rw_end   = nso_headers[i].rw_dst_offset   + nso_headers[i].rw_size + nso_headers[i].bss_size;
                    out->nso_size[i] = text_end;
                    out->nso_size[i] = std::max(out->nso_size[i], ro_end);
                    out->nso_size[i] = std::max(out->nso_size[i], rw_end);
                    out->nso_size[i] = (out->nso_size[i] + size_t(0xFFFul)) & ~size_t(0xFFFul);

                    total_size += out->nso_size[i];

                    if (arg_info != nullptr && arg_info->args_size && !out->args_size) {
                        out->args_address = total_size;
                        out->args_size = 2 * arg_info->args_size + args::ArgumentSizeMax + 2 * sizeof(u32);
                        out->args_size = (out->args_size + size_t(0xFFFul)) & ~size_t(0xFFFul);
                        total_size += out->args_size;
                    }
                }
            }

            /* Calculate ASLR. */
            uintptr_t aslr_start = 0;
            uintptr_t aslr_size  = 0;
            if (GetRuntimeFirmwareVersion() >= FirmwareVersion_200) {
                switch (out_cpi->flags & svc::CreateProcessFlag_AddressSpaceMask) {
                    case svc::CreateProcessFlag_AddressSpace32Bit:
                    case svc::CreateProcessFlag_AddressSpace32BitWithoutAlias:
                        aslr_start = map::AslrBase32Bit;
                        aslr_size  = map::AslrSize32Bit;
                        break;
                    case svc::CreateProcessFlag_AddressSpace64BitDeprecated:
                        aslr_start = map::AslrBase64BitDeprecated;
                        aslr_size  = map::AslrSize64BitDeprecated;
                        break;
                    case svc::CreateProcessFlag_AddressSpace64Bit:
                        aslr_start = map::AslrBase64Bit;
                        aslr_size  = map::AslrSize64Bit;
                        break;
                    default:
                        std::abort();
                }
            } else {
                /* On 1.0.0, only 2 address space types existed. */
                if (out_cpi->flags & svc::CreateProcessFlag_AddressSpace64BitDeprecated) {
                    aslr_start = map::AslrBase64BitDeprecated;
                    aslr_size  = map::AslrSize64BitDeprecated;
                } else {
                    aslr_start = map::AslrBase32Bit;
                    aslr_size  = map::AslrSize32Bit;
                }
            }
            if (total_size > aslr_size) {
                return ResultKernelOutOfMemory;
            }

            /* Set Create Process output. */
            uintptr_t aslr_slide = 0;
            uintptr_t unused_size = (aslr_size - total_size);
            if (out_cpi->flags & svc::CreateProcessFlag_EnableAslr) {
                aslr_slide = sts::rnd::GenerateRandomU64(unused_size / BaseAddressAlignment) * BaseAddressAlignment;
            }

            /* Set out. */
            aslr_start += aslr_slide;
            for (size_t i = 0; i < Nso_Count; i++) {
                if (has_nso[i]) {
                    out->nso_address[i] += aslr_start;
                }
            }
            if (out->args_address) {
                out->args_address += aslr_start;
            }

            out_cpi->code_address = aslr_start;
            out_cpi->code_num_pages = total_size >> 12;

            return ResultSuccess;
        }

        Result CreateProcessImpl(ProcessInfo *out, const Meta *meta, const NsoHeader *nso_headers, const bool *has_nso, const args::ArgumentInfo *arg_info, u32 flags, Handle reslimit_h) {
            /* Get CreateProcessInfo. */
            CreateProcessInfo cpi;
            R_TRY(GetCreateProcessInfo(&cpi, meta, flags, reslimit_h));

            /* Decide on an NSO layout. */
            R_TRY(DecideAddressSpaceLayout(out, &cpi, nso_headers, has_nso, arg_info));

            /* Actually create process. const_cast necessary because libnx doesn't declare svcCreateProcess with const u32*. */
            return svcCreateProcess(out->process_handle.GetPointer(), &cpi, reinterpret_cast<const u32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(u32));
        }

        Result LoadNsoSegment(FILE *f, const NsoHeader::SegmentInfo *segment, size_t file_size, const u8 *file_hash, bool is_compressed, bool check_hash, uintptr_t map_base, uintptr_t map_end) {
            /* Select read size based on compression. */
            if (!is_compressed) {
                file_size = segment->size;
            }

            /* Validate size. */
            if (file_size > segment->size || file_size > std::numeric_limits<s32>::max() || segment->size > std::numeric_limits<s32>::max()) {
                return ResultLoaderInvalidNso;
            }

            /* Load data from file. */
            uintptr_t load_address = is_compressed ? map_end - file_size : map_base;
            fseek(f, segment->file_offset, SEEK_SET);
            if (fread(reinterpret_cast<void *>(load_address), file_size, 1, f) != 1) {
                return ResultLoaderInvalidNso;
            }

            /* Uncompress if necessary. */
            if (is_compressed) {
                if (util::DecompressLZ4(reinterpret_cast<void *>(map_base), segment->size, reinterpret_cast<const void *>(load_address), file_size) != static_cast<int>(segment->size)) {
                    return ResultLoaderInvalidNso;
                }
            }

            /* Check hash if necessary. */
            if (check_hash) {
                u8 hash[SHA256_HASH_SIZE];
                sha256CalculateHash(hash, reinterpret_cast<void *>(map_base), segment->size);

                if (std::memcmp(hash, file_hash, sizeof(hash)) != 0) {
                    return ResultLoaderInvalidNso;
                }
            }

            return ResultSuccess;
        }

        Result LoadNsoIntoProcessMemory(Handle process_handle, FILE *f, uintptr_t map_address, const NsoHeader *nso_header, uintptr_t nso_address, size_t nso_size) {
            /* Map and read data from file. */
            {
                map::AutoCloseMap mapper(map_address, process_handle, nso_address, nso_size);
                R_TRY(mapper.GetResult());

                /* Load NSO segments. */
                R_TRY(LoadNsoSegment(f, &nso_header->segments[NsoHeader::Segment_Text], nso_header->text_compressed_size, nso_header->text_hash, (nso_header->flags & NsoHeader::Flag_CompressedText) != 0,
                                        (nso_header->flags & NsoHeader::Flag_CheckHashText) != 0, map_address + nso_header->text_dst_offset, map_address + nso_size));
                R_TRY(LoadNsoSegment(f, &nso_header->segments[NsoHeader::Segment_Ro], nso_header->ro_compressed_size, nso_header->ro_hash, (nso_header->flags & NsoHeader::Flag_CompressedRo) != 0,
                                        (nso_header->flags & NsoHeader::Flag_CheckHashRo) != 0, map_address + nso_header->ro_dst_offset, map_address + nso_size));
                R_TRY(LoadNsoSegment(f, &nso_header->segments[NsoHeader::Segment_Rw], nso_header->rw_compressed_size, nso_header->rw_hash, (nso_header->flags & NsoHeader::Flag_CompressedRw) != 0,
                                        (nso_header->flags & NsoHeader::Flag_CheckHashRw) != 0, map_address + nso_header->rw_dst_offset, map_address + nso_size));

                /* Clear unused space to zero. */
                const size_t text_end = nso_header->text_dst_offset + nso_header->text_size;
                const size_t ro_end   = nso_header->ro_dst_offset   + nso_header->ro_size;
                const size_t rw_end   = nso_header->rw_dst_offset   + nso_header->rw_size;
                std::memset(reinterpret_cast<void *>(map_address),            0, nso_header->text_dst_offset);
                std::memset(reinterpret_cast<void *>(map_address + text_end), 0, nso_header->ro_dst_offset - text_end);
                std::memset(reinterpret_cast<void *>(map_address + ro_end),   0, nso_header->rw_dst_offset - ro_end);
                std::memset(reinterpret_cast<void *>(map_address + rw_end), 0, nso_header->bss_size);

                /* Apply IPS patches. */
                LocateAndApplyIpsPatchesToModule(nso_header->build_id, map_address, nso_size);
            }

            /* Set permissions. */
            const size_t text_size = (static_cast<size_t>(nso_header->text_size) + size_t(0xFFFul)) & ~size_t(0xFFFul);
            const size_t ro_size = (static_cast<size_t>(nso_header->ro_size)   + size_t(0xFFFul)) & ~size_t(0xFFFul);
            const size_t rw_size = (static_cast<size_t>(nso_header->rw_size + nso_header->bss_size) + size_t(0xFFFul)) & ~size_t(0xFFFul);
            if (text_size) {
                R_TRY(svcSetProcessMemoryPermission(process_handle, nso_address + nso_header->text_dst_offset, text_size, Perm_Rx));
            }
            if (ro_size) {
                R_TRY(svcSetProcessMemoryPermission(process_handle, nso_address + nso_header->ro_dst_offset,   ro_size,   Perm_R));
            }
            if (rw_size) {
                R_TRY(svcSetProcessMemoryPermission(process_handle, nso_address + nso_header->rw_dst_offset,   rw_size,   Perm_Rw));
            }

            return ResultSuccess;
        }

        Result LoadNsosIntoProcessMemory(const ProcessInfo *process_info, const ncm::TitleId title_id, const NsoHeader *nso_headers, const bool *has_nso, const args::ArgumentInfo *arg_info) {
            const Handle process_handle = process_info->process_handle.Get();

            /* Load each NSO. */
            for (size_t i = 0; i < Nso_Count; i++) {
                if (has_nso[i]) {
                    FILE *f = nullptr;
                    R_TRY(OpenCodeFile(f, title_id, GetNsoName(i)));
                    ON_SCOPE_EXIT { fclose(f); };

                    uintptr_t map_address = 0;
                    R_TRY(map::LocateMappableSpace(&map_address, process_info->nso_size[i]));

                    R_TRY(LoadNsoIntoProcessMemory(process_handle, f, map_address, nso_headers + i, process_info->nso_address[i], process_info->nso_size[i]));
                }
            }

            /* Load arguments, if present. */
            if (arg_info != nullptr) {
                /* Write argument data into memory. */
                {
                    uintptr_t map_address = 0;
                    R_TRY(map::LocateMappableSpace(&map_address, process_info->args_size));

                    map::AutoCloseMap mapper(map_address, process_handle, process_info->args_address, process_info->args_size);
                    R_TRY(mapper.GetResult());

                    ProgramArguments *args = reinterpret_cast<ProgramArguments *>(map_address);
                    std::memset(args, 0, sizeof(*args));
                    args->allocated_size = process_info->args_size;
                    args->arguments_size = arg_info->args_size;
                    std::memcpy(args->arguments, arg_info->args, arg_info->args_size);
                }

                /* Set argument region permissions. */
                R_TRY(svcSetProcessMemoryPermission(process_handle, process_info->args_address, process_info->args_size, Perm_Rw));
            }

            return ResultSuccess;
        }

    }

    /* Process Creation API. */
    Result CreateProcess(Handle *out, PinId pin_id, const ncm::TitleLocation &loc, const char *path, u32 flags, Handle reslimit_h) {
        /* Use global storage for NSOs. */
        NsoHeader *nso_headers = g_nso_headers;
        bool *has_nso = g_has_nso;
        const auto arg_info = args::Get(loc.title_id);

        {
            /* Mount code. */
            ScopedCodeMount mount(loc);
            R_TRY(mount.GetResult());

            /* Load meta, possibly from cache. */
            Meta meta;
            R_TRY(LoadMetaFromCache(&meta, loc.title_id));

            /* Validate meta. */
            R_TRY(ValidateMeta(&meta, loc));

            /* Load, validate NSOs. */
            R_TRY(LoadNsoHeaders(loc.title_id, nso_headers, has_nso));
            R_TRY(ValidateNsoHeaders(nso_headers, has_nso));

            /* Actually create process. */
            ProcessInfo info;
            R_TRY(CreateProcessImpl(&info, &meta, nso_headers, has_nso, arg_info, flags, reslimit_h));

            /* Load NSOs into process memory. */
            R_TRY(LoadNsosIntoProcessMemory(&info, loc.title_id, nso_headers, has_nso, arg_info));

            /* Register NSOs with ro manager. */
            u64 process_id;
            {
                /* Nintendo doesn't validate this result, but we will. */
                R_ASSERT(svcGetProcessId(&process_id, info.process_handle.Get()));

                /* Register new process. */
                ldr::ro::RegisterProcess(pin_id, process_id, loc.title_id);

                /* Register all NSOs. */
                for (size_t i = 0; i < Nso_Count; i++) {
                    if (has_nso[i]) {
                        ldr::ro::RegisterModule(pin_id, nso_headers[i].build_id, info.nso_address[i], info.nso_size[i]);
                    }
                }
            }

            /* If we're overriding for HBL, perform HTML document redirection. */
            if (mount.IsHblMounted()) {
                /* Don't validate result, failure is okay. */
                RedirectHtmlDocumentPathForHbl(loc);
            }

            /* Clear the ECS entry for the title. */
            R_ASSERT(ecs::Clear(loc.title_id));

            /* Note that we've created the title. */
            SetLaunchedTitle(loc.title_id);

            /* Move the process handle to output. */
            *out = info.process_handle.Move();
        }

        return ResultSuccess;
    }

    Result GetProgramInfo(ProgramInfo *out, const ncm::TitleLocation &loc) {
        Meta meta;

        /* Load Meta. */
        {
            ScopedCodeMount mount(loc);
            R_TRY(mount.GetResult());
            R_TRY(LoadMeta(&meta, loc.title_id));
        }

        return GetProgramInfoFromMeta(out, &meta);
    }

}
