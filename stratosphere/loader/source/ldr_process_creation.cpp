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
#include "ldr_capabilities.hpp"
#include "ldr_content_management.hpp"
#include "ldr_development_manager.hpp"
#include "ldr_launch_record.hpp"
#include "ldr_meta.hpp"
#include "ldr_patcher.hpp"
#include "ldr_process_creation.hpp"
#include "ldr_ro_manager.hpp"

namespace ams::ldr {

    namespace {

        /* Convenience defines. */
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

        constexpr inline const char *NsoPaths[Nso_Count] = {
            ENCODE_ATMOSPHERE_CODE_PATH("/rtld"),
            ENCODE_ATMOSPHERE_CODE_PATH("/main"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk0"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk1"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk2"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk3"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk4"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk5"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk6"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk7"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk8"),
            ENCODE_ATMOSPHERE_CODE_PATH("/subsdk9"),
            ENCODE_ATMOSPHERE_CODE_PATH("/sdk"),
        };

        constexpr const char *GetNsoPath(size_t idx) {
            AMS_ABORT_UNLESS(idx < Nso_Count);
            return NsoPaths[idx];
        }

        struct ProcessInfo {
            os::ManagedHandle process_handle;
            uintptr_t args_address;
            size_t    args_size;
            uintptr_t nso_address[Nso_Count];
            size_t    nso_size[Nso_Count];
        };

        /* Global NSO header cache. */
        bool g_has_nso[Nso_Count];
        NsoHeader g_nso_headers[Nso_Count];

        /* Anti-downgrade. */
        #include "ldr_anti_downgrade_tables.inc"

        Result ValidateProgramVersion(ncm::ProgramId program_id, u32 version) {
            /* No version verification is done before 8.1.0. */
            R_SUCCEED_IF(hos::GetVersion() < hos::Version_8_1_0);

            /* No verification is done if development. */
            R_SUCCEED_IF(IsDevelopmentForAntiDowngradeCheck());

            /* Do version-dependent validation, if compiled to do so. */
#ifdef LDR_VALIDATE_PROCESS_VERSION
            const MinimumProgramVersion *entries = nullptr;
            size_t num_entries = 0;

            const auto hos_version = hos::GetVersion();
            if (hos_version >= hos::Version_11_0_0) {
                entries = g_MinimumProgramVersions1100;
                num_entries = g_MinimumProgramVersionsCount1100;
            } else if (hos_version >= hos::Version_10_1_0) {
                entries = g_MinimumProgramVersions1010;
                num_entries = g_MinimumProgramVersionsCount1010;
            } else if (hos_version >= hos::Version_10_0_0) {
                entries = g_MinimumProgramVersions1000;
                num_entries = g_MinimumProgramVersionsCount1000;
            } else if (hos_version >= hos::Version_9_1_0) {
                entries = g_MinimumProgramVersions910;
                num_entries = g_MinimumProgramVersionsCount910;
            } else if (hos_version >= hos::Version_9_0_0) {
                entries = g_MinimumProgramVersions900;
                num_entries = g_MinimumProgramVersionsCount900;
            } else if (hos_version >= hos::Version_8_1_0) {
                entries = g_MinimumProgramVersions810;
                num_entries = g_MinimumProgramVersionsCount810;
            }

            for (size_t i = 0; i < num_entries; i++) {
                if (entries[i].program_id == program_id) {
                    R_UNLESS(entries[i].version <= version, ResultInvalidVersion());
                }
            }
#endif
            return ResultSuccess();
        }

        /* Helpers. */
        Result GetProgramInfoFromMeta(ProgramInfo *out, const Meta *meta) {
            /* Copy basic info. */
            out->main_thread_priority = meta->npdm->main_thread_priority;
            out->default_cpu_id = meta->npdm->default_cpu_id;
            out->main_thread_stack_size = meta->npdm->main_thread_stack_size;
            out->program_id = meta->aci->program_id;

            /* Copy access controls. */
            size_t offset = 0;
#define COPY_ACCESS_CONTROL(source, which) \
            ({ \
                const size_t size = meta->source->which##_size; \
                R_UNLESS(offset + size <= sizeof(out->ac_buffer), ResultInternalError()); \
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
            return ResultSuccess();
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

        Result LoadNsoHeaders(NsoHeader *nso_headers, bool *has_nso) {
            /* Clear NSOs. */
            std::memset(nso_headers, 0, sizeof(*nso_headers) * Nso_Count);
            std::memset(has_nso, 0, sizeof(*has_nso) * Nso_Count);

            for (size_t i = 0; i < Nso_Count; i++) {
                fs::FileHandle file;
                if (R_SUCCEEDED(fs::OpenFile(std::addressof(file), GetNsoPath(i), fs::OpenMode_Read))) {
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Read NSO header. */
                    size_t read_size;
                    R_TRY(fs::ReadFile(std::addressof(read_size), file, 0, nso_headers + i, sizeof(*nso_headers)));
                    R_UNLESS(read_size == sizeof(*nso_headers), ResultInvalidNso());

                    has_nso[i] = true;
                }
            }

            return ResultSuccess();
        }

        Result ValidateNsoHeaders(const NsoHeader *nso_headers, const bool *has_nso) {
            /* We must always have a main. */
            R_UNLESS(has_nso[Nso_Main], ResultInvalidNso());

            /* If we don't have an RTLD, we must only have a main. */
            if (!has_nso[Nso_Rtld]) {
                for (size_t i = Nso_Main + 1; i < Nso_Count; i++) {
                    R_UNLESS(!has_nso[i], ResultInvalidNso());
                }
            }

            /* All NSOs must have zero text offset. */
            for (size_t i = 0; i < Nso_Count; i++) {
                R_UNLESS(nso_headers[i].text_dst_offset == 0, ResultInvalidNso());
            }

            return ResultSuccess();
        }

        Result ValidateMeta(const Meta *meta, const ncm::ProgramLocation &loc, const fs::CodeVerificationData &code_verification_data) {
            /* Validate version. */
            R_TRY(ValidateProgramVersion(loc.program_id, meta->npdm->version));

            /* Validate program id. */
            R_UNLESS(meta->aci->program_id >= meta->acid->program_id_min, ResultInvalidProgramId());
            R_UNLESS(meta->aci->program_id <= meta->acid->program_id_max, ResultInvalidProgramId());

            /* Validate the kernel capabilities. */
            R_TRY(caps::ValidateCapabilities(meta->acid_kac, meta->acid->kac_size, meta->aci_kac, meta->aci->kac_size));

            /* If we have data to validate, validate it. */
            if (code_verification_data.has_data && meta->check_verification_data) {
                const u8 *sig         = code_verification_data.signature;
                const size_t sig_size = sizeof(code_verification_data.signature);
                const u8 *mod         = static_cast<u8 *>(meta->modulus);
                const size_t mod_size = crypto::Rsa2048PssSha256Verifier::ModulusSize;
                const u8 *exp         = fssystem::GetAcidSignatureKeyPublicExponent();
                const size_t exp_size = fssystem::AcidSignatureKeyPublicExponentSize;
                const u8 *hsh         = code_verification_data.target_hash;
                const size_t hsh_size = sizeof(code_verification_data.target_hash);
                const bool is_signature_valid = crypto::VerifyRsa2048PssSha256WithHash(sig, sig_size, mod, mod_size, exp, exp_size, hsh, hsh_size);

                R_UNLESS(is_signature_valid, ResultInvalidNcaSignature());
            }

            /* All good. */
            return ResultSuccess();
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
                    if (hos::GetVersion() >= hos::Version_2_0_0 || svc::IsKernelMesosphere()) {
                        flags |= svc::CreateProcessFlag_AddressSpace64Bit;
                    } else {
                        flags |= svc::CreateProcessFlag_AddressSpace64BitDeprecated;
                    }
                    break;
                default:
                    return ResultInvalidMeta();
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
                if (hos::GetVersion() >= hos::Version_7_0_0) {
                    if (meta_flags & Npdm::MetaFlag_OptimizeMemoryAllocation) {
                        flags |= svc::CreateProcessFlag_OptimizeMemoryAllocation;
                    }
                }
            }

            /* 5.0.0+ Set Pool Partition. */
            if (hos::GetVersion() >= hos::Version_5_0_0) {
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
                        return ResultInvalidMeta();
                }
            } else if (hos::GetVersion() >= hos::Version_4_0_0) {
                /* On 4.0.0+, the corresponding bit was simply "UseSecureMemory". */
                if (meta->acid->flags & Acid::AcidFlag_DeprecatedUseSecureMemory) {
                    flags |= svc::CreateProcessFlag_DeprecatedUseSecureMemory;
                }
            }

            /* 11.0.0+ Set Disable DAS merge. */
            if (hos::GetVersion() >= hos::Version_11_0_0 || svc::IsKernelMesosphere()) {
                if (meta_flags & Npdm::MetaFlag_DisableDeviceAddressSpaceMerge) {
                    flags |= svc::CreateProcessFlag_DisableDeviceAddressSpaceMerge;
                }
            }

            *out = flags;
            return ResultSuccess();
        }

        Result GetCreateProcessParameter(svc::CreateProcessParameter *out, const Meta *meta, u32 flags, Handle reslimit_h) {
            /* Clear output. */
            std::memset(out, 0, sizeof(*out));

            /* Set name, version, program id, resource limit handle. */
            std::memcpy(out->name, meta->npdm->program_name, sizeof(out->name) - 1);
            out->version = meta->npdm->version;
            out->program_id = static_cast<u64>(meta->aci->program_id);
            out->reslimit = reslimit_h;

            /* Set flags. */
            R_TRY(GetCreateProcessFlags(&out->flags, meta, flags));

            /* 3.0.0+ System Resource Size. */
            if (hos::GetVersion() >= hos::Version_3_0_0) {
                /* Validate size is aligned. */
                R_UNLESS(util::IsAligned(meta->npdm->system_resource_size, os::MemoryBlockUnitSize), ResultInvalidSize());

                /* Validate system resource usage. */
                if (meta->npdm->system_resource_size) {
                    /* Process must be 64-bit. */
                    R_UNLESS((out->flags & svc::CreateProcessFlag_AddressSpace64Bit), ResultInvalidMeta());

                    /* Process must be application or applet. */
                    R_UNLESS(IsApplication(meta) || IsApplet(meta), ResultInvalidMeta());

                    /* Size must be less than or equal to max. */
                    R_UNLESS(meta->npdm->system_resource_size <= SystemResourceSizeMax, ResultInvalidMeta());
                }
                out->system_resource_num_pages = meta->npdm->system_resource_size >> 12;
            }

            return ResultSuccess();
        }

        Result DecideAddressSpaceLayout(ProcessInfo *out, svc::CreateProcessParameter *out_param, const NsoHeader *nso_headers, const bool *has_nso, const args::ArgumentInfo *arg_info) {
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
            if (hos::GetVersion() >= hos::Version_2_0_0) {
                switch (out_param->flags & svc::CreateProcessFlag_AddressSpaceMask) {
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
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            } else {
                /* On 1.0.0, only 2 address space types existed. */
                if (out_param->flags & svc::CreateProcessFlag_AddressSpace64BitDeprecated) {
                    aslr_start = map::AslrBase64BitDeprecated;
                    aslr_size  = map::AslrSize64BitDeprecated;
                } else {
                    aslr_start = map::AslrBase32Bit;
                    aslr_size  = map::AslrSize32Bit;
                }
            }
            R_UNLESS(total_size <= aslr_size, svc::ResultOutOfMemory());

            /* Set Create Process output. */
            uintptr_t aslr_slide = 0;
            uintptr_t free_size = (aslr_size - total_size);
            if (out_param->flags & svc::CreateProcessFlag_EnableAslr) {
                /* Nintendo uses MT19937 (not os::GenerateRandomBytes), but we'll just use TinyMT for now. */
                aslr_slide = os::GenerateRandomU64(free_size / os::MemoryBlockUnitSize) * os::MemoryBlockUnitSize;
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

            out_param->code_address = aslr_start;
            out_param->code_num_pages = total_size >> 12;

            return ResultSuccess();
        }

        Result CreateProcessImpl(ProcessInfo *out, const Meta *meta, const NsoHeader *nso_headers, const bool *has_nso, const args::ArgumentInfo *arg_info, u32 flags, Handle reslimit_h) {
            /* Get CreateProcessParameter. */
            svc::CreateProcessParameter param;
            R_TRY(GetCreateProcessParameter(std::addressof(param), meta, flags, reslimit_h));

            /* Decide on an NSO layout. */
            R_TRY(DecideAddressSpaceLayout(out, std::addressof(param), nso_headers, has_nso, arg_info));

            /* Actually create process. */
            Handle process_handle;
            R_TRY(svc::CreateProcess(std::addressof(process_handle), std::addressof(param), static_cast<const u32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(u32)));

            /* Set the output handle. */
            *out->process_handle.GetPointer() = process_handle;
            return ResultSuccess();
        }

        Result LoadNsoSegment(fs::FileHandle file, const NsoHeader::SegmentInfo *segment, size_t file_size, const u8 *file_hash, bool is_compressed, bool check_hash, uintptr_t map_base, uintptr_t map_end) {
            /* Select read size based on compression. */
            if (!is_compressed) {
                file_size = segment->size;
            }

            /* Validate size. */
            R_UNLESS(file_size <= segment->size,                       ResultInvalidNso());
            R_UNLESS(segment->size <= std::numeric_limits<s32>::max(), ResultInvalidNso());

            /* Load data from file. */
            uintptr_t load_address = is_compressed ? map_end - file_size : map_base;
            size_t read_size;
            R_TRY(fs::ReadFile(std::addressof(read_size), file, segment->file_offset, reinterpret_cast<void *>(load_address), file_size));
            R_UNLESS(read_size == file_size, ResultInvalidNso());

            /* Uncompress if necessary. */
            if (is_compressed) {
                bool decompressed = (util::DecompressLZ4(reinterpret_cast<void *>(map_base), segment->size, reinterpret_cast<const void *>(load_address), file_size) == static_cast<int>(segment->size));
                R_UNLESS(decompressed, ResultInvalidNso());
            }

            /* Check hash if necessary. */
            if (check_hash) {
                u8 hash[crypto::Sha256Generator::HashSize];
                crypto::GenerateSha256Hash(hash, sizeof(hash), reinterpret_cast<void *>(map_base), segment->size);

                R_UNLESS(std::memcmp(hash, file_hash, sizeof(hash)) == 0, ResultInvalidNso());
            }

            return ResultSuccess();
        }

        Result LoadNsoIntoProcessMemory(Handle process_handle, fs::FileHandle file, uintptr_t map_address, const NsoHeader *nso_header, uintptr_t nso_address, size_t nso_size) {
            /* Map and read data from file. */
            {
                map::AutoCloseMap mapper(map_address, process_handle, nso_address, nso_size);
                R_TRY(mapper.GetResult());

                /* Load NSO segments. */
                R_TRY(LoadNsoSegment(file, &nso_header->segments[NsoHeader::Segment_Text], nso_header->text_compressed_size, nso_header->text_hash, (nso_header->flags & NsoHeader::Flag_CompressedText) != 0,
                                           (nso_header->flags & NsoHeader::Flag_CheckHashText) != 0, map_address + nso_header->text_dst_offset, map_address + nso_size));
                R_TRY(LoadNsoSegment(file, &nso_header->segments[NsoHeader::Segment_Ro], nso_header->ro_compressed_size, nso_header->ro_hash, (nso_header->flags & NsoHeader::Flag_CompressedRo) != 0,
                                           (nso_header->flags & NsoHeader::Flag_CheckHashRo) != 0, map_address + nso_header->ro_dst_offset, map_address + nso_size));
                R_TRY(LoadNsoSegment(file, &nso_header->segments[NsoHeader::Segment_Rw], nso_header->rw_compressed_size, nso_header->rw_hash, (nso_header->flags & NsoHeader::Flag_CompressedRw) != 0,
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

            return ResultSuccess();
        }

        Result LoadNsosIntoProcessMemory(const ProcessInfo *process_info, const NsoHeader *nso_headers, const bool *has_nso, const args::ArgumentInfo *arg_info) {
            const Handle process_handle = process_info->process_handle.Get();

            /* Load each NSO. */
            for (size_t i = 0; i < Nso_Count; i++) {
                if (has_nso[i]) {
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), GetNsoPath(i), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    uintptr_t map_address = 0;
                    R_TRY(map::LocateMappableSpace(&map_address, process_info->nso_size[i]));

                    R_TRY(LoadNsoIntoProcessMemory(process_handle, file, map_address, nso_headers + i, process_info->nso_address[i], process_info->nso_size[i]));
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

            return ResultSuccess();
        }

    }

    /* Process Creation API. */
    Result CreateProcess(Handle *out, PinId pin_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status, const char *path, u32 flags, Handle reslimit_h) {
        /* Use global storage for NSOs. */
        NsoHeader *nso_headers = g_nso_headers;
        bool *has_nso = g_has_nso;
        const auto arg_info = args::Get(loc.program_id);

        {
            /* Mount code. */
            ScopedCodeMount mount(loc, override_status);
            R_TRY(mount.GetResult());

            /* Load meta, possibly from cache. */
            Meta meta;
            R_TRY(LoadMetaFromCache(&meta, loc, override_status));

            /* Validate meta. */
            R_TRY(ValidateMeta(&meta, loc, mount.GetCodeVerificationData()));

            /* Load, validate NSOs. */
            R_TRY(LoadNsoHeaders(nso_headers, has_nso));
            R_TRY(ValidateNsoHeaders(nso_headers, has_nso));

            /* Actually create process. */
            ProcessInfo info;
            R_TRY(CreateProcessImpl(&info, &meta, nso_headers, has_nso, arg_info, flags, reslimit_h));

            /* Load NSOs into process memory. */
            R_TRY(LoadNsosIntoProcessMemory(&info, nso_headers, has_nso, arg_info));

            /* Register NSOs with ro manager. */
            {
                /* Nintendo doesn't validate this get, but we do. */
                os::ProcessId process_id = os::GetProcessId(info.process_handle.Get());

                /* Register new process. */
                ldr::ro::RegisterProcess(pin_id, process_id, loc.program_id);

                /* Register all NSOs. */
                for (size_t i = 0; i < Nso_Count; i++) {
                    if (has_nso[i]) {
                        ldr::ro::RegisterModule(pin_id, nso_headers[i].build_id, info.nso_address[i], info.nso_size[i]);
                    }
                }
            }

            /* If we're overriding for HBL, perform HTML document redirection. */
            if (override_status.IsHbl()) {
                /* Don't validate result, failure is okay. */
                RedirectHtmlDocumentPathForHbl(loc);
            }

            /* Clear the external code for the program. */
            fssystem::DestroyExternalCode(loc.program_id);

            /* Note that we've created the program. */
            SetLaunchedProgram(loc.program_id);

            /* Move the process handle to output. */
            *out = info.process_handle.Move();
        }

        return ResultSuccess();
    }

    Result GetProgramInfo(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc) {
        Meta meta;

        /* Load Meta. */
        {
            ScopedCodeMount mount(loc);
            R_TRY(mount.GetResult());
            R_TRY(LoadMeta(&meta, loc, mount.GetOverrideStatus()));
            if (out_status != nullptr) {
                *out_status = mount.GetOverrideStatus();
            }
        }

        return GetProgramInfoFromMeta(out, &meta);
    }

}
