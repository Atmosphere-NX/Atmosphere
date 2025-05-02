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
            Nso_Compat0 =  2,
            Nso_Compat1 =  3,
            Nso_Compat2 =  4,
            Nso_Compat3 =  5,
            Nso_Compat4 =  6,
            Nso_Compat5 =  7,
            Nso_Compat6 =  8,
            Nso_Compat7 =  9,
            Nso_Compat8 = 10,
            Nso_Compat9 = 11,
            Nso_SubSdk0 = 12,
            Nso_SubSdk1 = 13,
            Nso_SubSdk2 = 14,
            Nso_SubSdk3 = 15,
            Nso_SubSdk4 = 16,
            Nso_SubSdk5 = 17,
            Nso_SubSdk6 = 18,
            Nso_SubSdk7 = 19,
            Nso_SubSdk8 = 20,
            Nso_SubSdk9 = 21,
            Nso_Sdk     = 22,
            Nso_Count,
        };

        constexpr inline const char *NsoPaths[Nso_Count] = {
            ENCODE_ATMOSPHERE_CODE_PATH("/rtld"),
            ENCODE_ATMOSPHERE_CODE_PATH("/main"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat0"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat1"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat2"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat3"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat4"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat5"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat6"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat7"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat8"),
            ENCODE_ATMOSPHERE_CMPT_PATH("/compat9"),
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
            os::NativeHandle process_handle;
            uintptr_t args_address;
            size_t    args_size;
            uintptr_t nso_address[Nso_Count];
            size_t    nso_size[Nso_Count];
        };

        /* Global NSO header cache. */
        bool g_has_nso[Nso_Count];
        NsoHeader g_nso_headers[Nso_Count];

        Result ValidateProgramVersion(ncm::ProgramId program_id, u32 version) {
            /* No version verification is done before 8.1.0. */
            R_SUCCEED_IF(hos::GetVersion() < hos::Version_8_1_0);

            /* No verification is done if development. */
            R_SUCCEED_IF(IsDevelopmentForAntiDowngradeCheck());

            /* TODO: Anti-downgrade checking does not make very much sense for us. Should we do anything? */
            AMS_UNUSED(program_id, version);

            R_SUCCEED();
        }

        /* Helpers. */
        Result GetProgramInfoFromMeta(ProgramInfo *out, const Meta *meta) {
            /* Copy basic info. */
            out->main_thread_priority   = meta->npdm->main_thread_priority;
            out->default_cpu_id         = meta->npdm->default_cpu_id;
            out->main_thread_stack_size = meta->npdm->main_thread_stack_size;
            out->program_id             = meta->aci->program_id;

            /* Copy access controls. */
            size_t offset = 0;
#define COPY_ACCESS_CONTROL(source, which) \
            ({ \
                const size_t size = meta->source->which##_size; \
                R_UNLESS(offset + size <= sizeof(out->ac_buffer), ldr::ResultInternalError()); \
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
            out->flags = MakeProgramInfoFlag(static_cast<const util::BitPack32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(util::BitPack32));
            R_SUCCEED();
        }

        bool IsApplet(const Meta *meta) {
            return (MakeProgramInfoFlag(static_cast<const util::BitPack32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(util::BitPack32)) & ProgramInfoFlag_ApplicationTypeMask) == ProgramInfoFlag_Applet;
        }

        bool IsApplication(const Meta *meta) {
            return (MakeProgramInfoFlag(static_cast<const util::BitPack32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(util::BitPack32)) & ProgramInfoFlag_ApplicationTypeMask) == ProgramInfoFlag_Application;
        }

        Npdm::AddressSpaceType GetAddressSpaceType(const Meta *meta) {
            return static_cast<Npdm::AddressSpaceType>((meta->npdm->flags & Npdm::MetaFlag_AddressSpaceTypeMask) >> Npdm::MetaFlag_AddressSpaceTypeShift);
        }

        Acid::PoolPartition GetPoolPartition(const Meta *meta) {
            return static_cast<Acid::PoolPartition>((meta->acid->flags & Acid::AcidFlag_PoolPartitionMask) >> Acid::AcidFlag_PoolPartitionShift);
        }

        Result LoadAutoLoadHeaders(NsoHeader *nso_headers, bool *has_nso) {
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
                    R_UNLESS(read_size == sizeof(*nso_headers), ldr::ResultInvalidNso());

                    has_nso[i] = true;
                }
            }

            R_SUCCEED();
        }

        Result CheckAutoLoad(const NsoHeader *nso_headers, const bool *has_nso) {
            /* We must always have a main. */
            R_UNLESS(has_nso[Nso_Main], ldr::ResultInvalidNso());

            /* If we don't have an RTLD, we must only have a main. */
            if (!has_nso[Nso_Rtld]) {
                for (size_t i = Nso_Main + 1; i < Nso_Count; i++) {
                    R_UNLESS(!has_nso[i], ldr::ResultInvalidNso());
                }
            }

            /* All NSOs must have zero text offset. */
            for (size_t i = 0; i < Nso_Count; i++) {
                R_UNLESS(nso_headers[i].text_dst_offset == 0, ldr::ResultInvalidNso());
            }

            R_SUCCEED();
        }

        constexpr const ncm::ProgramId UnqualifiedApprovalProgramIds[] = {
            { 0x010003F003A34000 }, /* Pokemon: Let's Go, Pikachu! */
            { 0x0100152000022000 }, /* Mario Kart 8 Deluxe */
            { 0x0100165003504000 }, /* Nintendo Labo Toy-Con 04: VR Kit */
            { 0x0100187003A36000 }, /* Pokemon: Let's Go, Eevee! */
            { 0x01002E5008C56000 }, /* Pokemon Sword [Live Tournament] */
            { 0x01002FF008C24000 }, /* Ring Fit Adventure */
            { 0x010049900F546001 }, /* Super Mario 3D All-Stars: Super Mario 64 */
            { 0x010057D00ECE4000 }, /* Nintendo Switch Online (Nintendo 64) [for Japan] */
            { 0x01006F8002326000 }, /* Animal Crossing: New Horizons */
            { 0x01006FB00F50E000 }, /* [???] */
            { 0x010070300F50C000 }, /* [???] */
            { 0x010075100E8EC000 }, /* 马力欧卡丁车8 豪华版 [Mario Kart 8 Deluxe for China] */
            { 0x01008DB008C2C000 }, /* Pokemon Shield */
            { 0x01009AD008C4C000 }, /* Pokemon: Let's Go, Pikachu! [Kiosk] */
            { 0x0100A66003384000 }, /* Hulu */
            { 0x0100ABF008968000 }, /* Pokemon Sword */
            { 0x0100C9A00ECE6000 }, /* Nintendo Switch Online (Nintendo 64) [for America] */
            { 0x0100ED100BA3A000 }, /* Mario Kart Live: Home Circuit */
            { 0x0100F38011CFE000 }, /* Animal Crossing: New Horizons Island Transfer Tool */
            { 0x0100F6B011028000 }, /* 健身环大冒险 [Ring Fit Adventure for China] */
        };

        /* Check that the unqualified approval programs are sorted. */
        static_assert([]() -> bool {
            for (size_t i = 0; i < util::size(UnqualifiedApprovalProgramIds) - 1; ++i) {
                if (UnqualifiedApprovalProgramIds[i].value >= UnqualifiedApprovalProgramIds[i + 1].value) {
                    return false;
                }
            }

            return true;
        }());

        bool IsUnqualifiedApprovalProgramId(ncm::ProgramId program_id) {
            /* Check if the program id is one with unqualified approval. */
            return std::binary_search(std::begin(UnqualifiedApprovalProgramIds), std::end(UnqualifiedApprovalProgramIds), program_id);
        }

        bool IsUnqualifiedApproval(const Meta *meta) {
            /* If the meta has unqualified approval flag, it's unqualified approval. */
            if (meta->acid->flags & ldr::Acid::AcidFlag_UnqualifiedApproval) {
                return true;
            }

            /* If the unqualified approval flag is not set, the program must be an application. */
            if (!IsApplication(meta)) {
                return false;
            }

            /* The program id must be a force unqualified approval program id. */
            return IsUnqualifiedApprovalProgramId(meta->acid->program_id_min) && meta->acid->program_id_min == meta->acid->program_id_max;
        }

        Result ValidateMeta(const Meta *meta, const ncm::ProgramLocation &loc, const fs::CodeVerificationData &code_verification_data) {
            /* Validate version. */
            R_TRY(ValidateProgramVersion(loc.program_id, meta->npdm->version));

            /* Validate program id. */
            R_UNLESS(meta->aci->program_id >= meta->acid->program_id_min, ldr::ResultInvalidProgramId());
            R_UNLESS(meta->aci->program_id <= meta->acid->program_id_max, ldr::ResultInvalidProgramId());

            /* Validate the kernel capabilities. */
            R_TRY(TestCapability(static_cast<const util::BitPack32 *>(meta->acid_kac), meta->acid->kac_size / sizeof(util::BitPack32), static_cast<const util::BitPack32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(util::BitPack32)));

            /* If we have data to validate, validate it. */
            if (meta->check_verification_data) {
                const u8 *sig         = code_verification_data.signature;
                const size_t sig_size = sizeof(code_verification_data.signature);
                const u8 *mod         = static_cast<u8 *>(meta->modulus);
                const size_t mod_size = crypto::Rsa2048PssSha256Verifier::ModulusSize;
                const u8 *exp         = fssystem::GetAcidSignatureKeyPublicExponent();
                const size_t exp_size = fssystem::AcidSignatureKeyPublicExponentSize;
                const u8 *hsh         = code_verification_data.target_hash;
                const size_t hsh_size = sizeof(code_verification_data.target_hash);
                const bool is_signature_valid = crypto::VerifyRsa2048PssSha256WithHash(sig, sig_size, mod, mod_size, exp, exp_size, hsh, hsh_size);

                /* If the signature check fails, we need to check if this is allowable. */
                if (!is_signature_valid) {
                    /* We have to enforce signature checks on prod and when we have a signature to check on dev. */
                    R_UNLESS(IsDevelopmentForAcidProductionCheck(), ldr::ResultInvalidNcaSignature());
                    R_UNLESS(!code_verification_data.has_data,      ldr::ResultInvalidNcaSignature());

                    /* There was no signature to check on dev. Check if this is acceptable. */
                    R_UNLESS(IsUnqualifiedApproval(meta), ldr::ResultInvalidNcaSignature());
                }
            }

            /* All good. */
            R_SUCCEED();
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
                    R_THROW(ldr::ResultInvalidMeta());
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
                /* TODO: Nintendo no longer accepts Applet when pool partition == application. Would this break hbl/anything else in the hb ecosystem? */
                /* TODO: Nintendo uses a helper bool MakeSvcPoolPartitionFlag(u32 *out, Acid::PoolPartition partition); */
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
                        R_THROW(ldr::ResultInvalidMeta());
                }
            } else if (hos::GetVersion() >= hos::Version_4_0_0) {
                /* On 4.0.0+, the corresponding bit was simply "UseSecureMemory". */
                if (meta->acid->flags & Acid::AcidFlag_DeprecatedUseSecureMemory) {
                    flags |= svc::CreateProcessFlag_DeprecatedUseSecureMemory;
                }
            }

            /* 11.0.0+/meso Set Disable DAS merge. */
            if (meta_flags & Npdm::MetaFlag_DisableDeviceAddressSpaceMerge) {
                flags |= svc::CreateProcessFlag_DisableDeviceAddressSpaceMerge;
            }

            /* 18.0.0+/meso Set Alias region extra size. */
            if (meta_flags & Npdm::MetaFlag_EnableAliasRegionExtraSize) {
                flags |= svc::CreateProcessFlag_EnableAliasRegionExtraSize;
            }

            *out = flags;
            R_SUCCEED();
        }

        Result GetCreateProcessParameter(svc::CreateProcessParameter *out, const Meta *meta, u32 flags, os::NativeHandle resource_limit) {
            /* Clear output. */
            std::memset(out, 0, sizeof(*out));

            /* Set name, version, program id, resource limit handle. */
            std::memcpy(out->name, meta->npdm->program_name, sizeof(out->name) - 1);
            out->version    = meta->npdm->version;
            out->program_id = meta->aci->program_id.value;
            out->reslimit   = resource_limit;

            /* Set flags. */
            R_TRY(GetCreateProcessFlags(std::addressof(out->flags), meta, flags));

            /* 3.0.0+ System Resource Size. */
            if (hos::GetVersion() >= hos::Version_3_0_0) {
                /* Validate size is aligned. */
                R_UNLESS(util::IsAligned(meta->npdm->system_resource_size, os::MemoryBlockUnitSize), ldr::ResultInvalidSize());

                /* Validate system resource usage. */
                if (meta->npdm->system_resource_size) {
                    /* Process must be 64-bit. */
                    R_UNLESS((out->flags & svc::CreateProcessFlag_AddressSpace64Bit), ldr::ResultInvalidMeta());

                    /* Process must be application or applet. */
                    R_UNLESS(IsApplication(meta) || IsApplet(meta), ldr::ResultInvalidMeta());

                    /* Size must be less than or equal to max. */
                    R_UNLESS(meta->npdm->system_resource_size <= SystemResourceSizeMax, ldr::ResultInvalidMeta());
                }
                out->system_resource_num_pages = meta->npdm->system_resource_size >> 12;
            }

            R_SUCCEED();
        }

        u64 GenerateSecureRandom(u64 max) {
            /* Generate a cryptographically random number. */
            u64 rand;
            crypto::GenerateCryptographicallyRandomBytes(std::addressof(rand), sizeof(rand));

            /* Coerce into range. */
            return rand % (max + 1);
        }

        Result DecideAddressSpaceLayout(ProcessInfo *out, svc::CreateProcessParameter *out_param, const NsoHeader *nso_headers, const bool *has_nso, const ArgumentStore::Entry *argument) {
            /* Clear output. */
            out->args_address = 0;
            out->args_size = 0;
            std::memset(out->nso_address, 0, sizeof(out->nso_address));
            std::memset(out->nso_size, 0, sizeof(out->nso_size));

            size_t total_size = 0;
            bool argument_allocated = false;

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
                    out->nso_size[i] = util::AlignUp(out->nso_size[i], os::MemoryPageSize);

                    total_size += out->nso_size[i];

                    if (!argument_allocated && argument != nullptr) {
                        out->args_address = total_size;
                        out->args_size    = util::AlignUp(2 * sizeof(u32) + argument->argument_size * 2 + ArgumentStore::ArgumentBufferSize, os::MemoryPageSize);
                        total_size += out->args_size;
                        argument_allocated = true;
                    }
                }
            }

            /* Calculate ASLR. */
            uintptr_t aslr_start = 0;
            size_t aslr_size     = 0;
            if (hos::GetVersion() >= hos::Version_2_0_0) {
                switch (out_param->flags & svc::CreateProcessFlag_AddressSpaceMask) {
                    case svc::CreateProcessFlag_AddressSpace32Bit:
                    case svc::CreateProcessFlag_AddressSpace32BitWithoutAlias:
                        aslr_start = svc::AddressSmallMap32Start;
                        aslr_size  = svc::AddressSmallMap32Size;
                        break;
                    case svc::CreateProcessFlag_AddressSpace64BitDeprecated:
                        aslr_start = svc::AddressSmallMap36Start;
                        aslr_size  = svc::AddressSmallMap36Size;
                        break;
                    case svc::CreateProcessFlag_AddressSpace64Bit:
                        aslr_start = svc::AddressMap39Start;
                        aslr_size  = svc::AddressMap39Size;
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            } else {
                /* On 1.0.0, only 2 address space types existed. */
                if (out_param->flags & svc::CreateProcessFlag_AddressSpace64BitDeprecated) {
                    aslr_start = svc::AddressSmallMap36Start;
                    aslr_size  = svc::AddressSmallMap36Size;
                } else {
                    aslr_start = svc::AddressSmallMap32Start;
                    aslr_size  = svc::AddressSmallMap32Size;
                }
            }
            R_UNLESS(total_size <= aslr_size, svc::ResultOutOfMemory());

            /* Set Create Process output. */
            uintptr_t aslr_slide = 0;
            size_t free_size     = (aslr_size - total_size);
            if (out_param->flags & svc::CreateProcessFlag_EnableAslr) {
                aslr_slide = GenerateSecureRandom(free_size / os::MemoryBlockUnitSize) * os::MemoryBlockUnitSize;
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

            out_param->code_address   = aslr_start;
            out_param->code_num_pages = total_size >> 12;

            R_SUCCEED();
        }

        Result LoadAutoLoadModuleSegment(fs::FileHandle file, const NsoHeader::SegmentInfo *segment, size_t file_size, const u8 *file_hash, bool is_compressed, bool check_hash, uintptr_t map_base, uintptr_t map_end) {
            /* Select read size based on compression. */
            if (!is_compressed) {
                file_size = segment->size;
            }

            /* Validate size. */
            R_UNLESS(file_size <= segment->size,                       ldr::ResultInvalidNso());
            R_UNLESS(segment->size <= std::numeric_limits<s32>::max(), ldr::ResultInvalidNso());

            /* Load data from file. */
            uintptr_t load_address = is_compressed ? map_end - file_size : map_base;
            size_t read_size;
            R_TRY(fs::ReadFile(std::addressof(read_size), file, segment->file_offset, reinterpret_cast<void *>(load_address), file_size));
            R_UNLESS(read_size == file_size, ldr::ResultInvalidNso());

            /* Uncompress if necessary. */
            if (is_compressed) {
                bool decompressed = (util::DecompressLZ4(reinterpret_cast<void *>(map_base), segment->size, reinterpret_cast<const void *>(load_address), file_size) == static_cast<int>(segment->size));
                R_UNLESS(decompressed, ldr::ResultInvalidNso());
            }

            /* Check hash if necessary. */
            if (check_hash) {
                u8 hash[crypto::Sha256Generator::HashSize];
                crypto::GenerateSha256(hash, sizeof(hash), reinterpret_cast<void *>(map_base), segment->size);

                R_UNLESS(std::memcmp(hash, file_hash, sizeof(hash)) == 0, ldr::ResultInvalidNso());
            }

            R_SUCCEED();
        }

        Result LoadAutoLoadModule(os::NativeHandle process_handle, fs::FileHandle file, const NsoHeader *nso_header, uintptr_t nso_address, size_t nso_size, bool prevent_code_reads) {
            /* Map and read data from file. */
            {
                /* Map the process memory. */
                void *mapped_memory = nullptr;
                R_TRY(os::MapProcessMemory(std::addressof(mapped_memory), process_handle, nso_address, nso_size, GenerateSecureRandom));
                ON_SCOPE_EXIT { os::UnmapProcessMemory(mapped_memory, process_handle, nso_address, nso_size); };

                const uintptr_t map_address = reinterpret_cast<uintptr_t>(mapped_memory);

                /* Load NSO segments. */
                R_TRY(LoadAutoLoadModuleSegment(file, std::addressof(nso_header->segments[NsoHeader::Segment_Text]), nso_header->text_compressed_size, nso_header->text_hash, (nso_header->flags & NsoHeader::Flag_CompressedText) != 0,
                                                      (nso_header->flags & NsoHeader::Flag_CheckHashText) != 0, map_address + nso_header->text_dst_offset, map_address + nso_size));
                R_TRY(LoadAutoLoadModuleSegment(file, std::addressof(nso_header->segments[NsoHeader::Segment_Ro]), nso_header->ro_compressed_size, nso_header->ro_hash, (nso_header->flags & NsoHeader::Flag_CompressedRo) != 0,
                                                      (nso_header->flags & NsoHeader::Flag_CheckHashRo) != 0, map_address + nso_header->ro_dst_offset, map_address + nso_size));
                R_TRY(LoadAutoLoadModuleSegment(file, std::addressof(nso_header->segments[NsoHeader::Segment_Rw]), nso_header->rw_compressed_size, nso_header->rw_hash, (nso_header->flags & NsoHeader::Flag_CompressedRw) != 0,
                                                      (nso_header->flags & NsoHeader::Flag_CheckHashRw) != 0, map_address + nso_header->rw_dst_offset, map_address + nso_size));

                /* Clear unused space to zero. */
                const size_t text_end = nso_header->text_dst_offset + nso_header->text_size;
                const size_t ro_end   = nso_header->ro_dst_offset   + nso_header->ro_size;
                const size_t rw_end   = nso_header->rw_dst_offset   + nso_header->rw_size;
                std::memset(reinterpret_cast<void *>(map_address + 0),        0, nso_header->text_dst_offset);
                std::memset(reinterpret_cast<void *>(map_address + text_end), 0, nso_header->ro_dst_offset - text_end);
                std::memset(reinterpret_cast<void *>(map_address + ro_end),   0, nso_header->rw_dst_offset - ro_end);
                std::memset(reinterpret_cast<void *>(map_address + rw_end),   0, nso_header->bss_size);

                /* Apply embedded patches. */
                ApplyEmbeddedPatchesToModule(nso_header->module_id, map_address, nso_size);

                /* Apply IPS patches. */
                LocateAndApplyIpsPatchesToModule(nso_header->module_id, map_address, nso_size);
            }

            /* Set permissions. */
            const size_t text_size = util::AlignUp(nso_header->text_size, os::MemoryPageSize);
            const size_t ro_size   = util::AlignUp(nso_header->ro_size, os::MemoryPageSize);
            const size_t rw_size   = util::AlignUp(nso_header->rw_size + nso_header->bss_size, os::MemoryPageSize);
            if (text_size) {
                R_TRY(os::SetProcessMemoryPermission(process_handle, nso_address + nso_header->text_dst_offset, text_size, prevent_code_reads ? os::MemoryPermission_ExecuteOnly : os::MemoryPermission_ReadExecute));
            }
            if (ro_size) {
                R_TRY(os::SetProcessMemoryPermission(process_handle, nso_address + nso_header->ro_dst_offset,   ro_size,   os::MemoryPermission_ReadOnly));
            }
            if (rw_size) {
                R_TRY(os::SetProcessMemoryPermission(process_handle, nso_address + nso_header->rw_dst_offset,   rw_size,   os::MemoryPermission_ReadWrite));
            }

            R_SUCCEED();
        }

        Result LoadAutoLoadModules(const ProcessInfo *process_info, const NsoHeader *nso_headers, const bool *has_nso, const ArgumentStore::Entry *argument, bool prevent_code_reads) {
            /* Load each NSO. */
            for (size_t i = 0; i < Nso_Count; i++) {
                if (has_nso[i]) {
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), GetNsoPath(i), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    R_TRY(LoadAutoLoadModule(process_info->process_handle, file, nso_headers + i, process_info->nso_address[i], process_info->nso_size[i], prevent_code_reads));
                }
            }

            /* Load arguments, if present. */
            if (argument != nullptr) {
                /* Write argument data into memory. */
                {
                    void *map_address = nullptr;
                    R_TRY(os::MapProcessMemory(std::addressof(map_address), process_info->process_handle, process_info->args_address, process_info->args_size, GenerateSecureRandom));
                    ON_SCOPE_EXIT { os::UnmapProcessMemory(map_address, process_info->process_handle, process_info->args_address, process_info->args_size); };

                    ProgramArguments *args = static_cast<ProgramArguments *>(map_address);
                    std::memset(args, 0, sizeof(*args));
                    args->allocated_size = process_info->args_size;
                    args->arguments_size = argument->argument_size;
                    std::memcpy(args->arguments, argument->argument, argument->argument_size);
                }

                /* Set argument region permissions. */
                /* NOTE: Nintendo uses svc::SetProcessMemoryPermission directly here. */
                R_TRY(os::SetProcessMemoryPermission(process_info->process_handle, process_info->args_address, process_info->args_size, os::MemoryPermission_ReadWrite));
            }

            R_SUCCEED();
        }

        Result CreateProcessAndLoadAutoLoadModules(ProcessInfo *out, const Meta *meta, const NsoHeader *nso_headers, const bool *has_nso, const ArgumentStore::Entry *argument, u32 flags, os::NativeHandle resource_limit) {
            /* Get CreateProcessParameter. */
            svc::CreateProcessParameter param;
            R_TRY(GetCreateProcessParameter(std::addressof(param), meta, flags, resource_limit));

            /* Decide on an NSO layout. */
            R_TRY(DecideAddressSpaceLayout(out, std::addressof(param), nso_headers, has_nso, argument));

            /* Actually create process. */
            svc::Handle process_handle;
            R_TRY(svc::CreateProcess(std::addressof(process_handle), std::addressof(param), static_cast<const u32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(u32)));

            /* Set the output handle, and ensure that if we fail after this point we clean it up. */
            out->process_handle = process_handle;
            ON_RESULT_FAILURE { svc::CloseHandle(process_handle); };

            /* Load all auto load modules. */
            R_RETURN(LoadAutoLoadModules(out, nso_headers, has_nso, argument, (meta->npdm->flags & ldr::Npdm::MetaFlag_PreventCodeReads) != 0));
        }

    }

    /* Process Creation API. */
    Result CreateProcess(os::NativeHandle *out, PinId pin_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status, const char *path, const ArgumentStore::Entry *argument, u32 flags, os::NativeHandle resource_limit, const ldr::ProgramAttributes &attrs) {
        /* Mount code. */
        AMS_UNUSED(path);
        ScopedCodeMount mount(loc, override_status, attrs);
        R_TRY(mount.GetResult());

        /* Load meta, possibly from cache. */
        Meta meta;
        R_TRY(LoadMetaFromCache(std::addressof(meta), loc, override_status, attrs.platform));

        /* Validate meta. */
        R_TRY(ValidateMeta(std::addressof(meta), loc, mount.GetCodeVerificationData()));

        /* Load, validate NSO headers. */
        R_TRY(LoadAutoLoadHeaders(g_nso_headers, g_has_nso));
        R_TRY(CheckAutoLoad(g_nso_headers, g_has_nso));

        /* Actually create the process and load NSOs into process memory. */
        ProcessInfo info;
        R_TRY(CreateProcessAndLoadAutoLoadModules(std::addressof(info), std::addressof(meta), g_nso_headers, g_has_nso, argument, flags, resource_limit));

        /* Register NSOs with the RoManager. */
        {
            /* Nintendo doesn't validate this get, but we do. */
            os::ProcessId process_id = os::GetProcessId(info.process_handle);

            /* Register new process. */
            const auto as_type = GetAddressSpaceType(std::addressof(meta));
            RoManager::GetInstance().RegisterProcess(pin_id, process_id, meta.aci->program_id, as_type == Npdm::AddressSpaceType_64Bit || as_type == Npdm::AddressSpaceType_64BitDeprecated);

            /* Register all NSOs. */
            for (size_t i = 0; i < Nso_Count; i++) {
                if (g_has_nso[i]) {
                    RoManager::GetInstance().AddNso(pin_id, g_nso_headers[i].module_id, info.nso_address[i], info.nso_size[i]);
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
        SetLaunchedBootProgram(loc.program_id);

        /* Move the process handle to output. */
        *out = info.process_handle;

        R_SUCCEED();
    }

    Result GetProgramInfo(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc, const char *path, const ldr::ProgramAttributes &attrs) {
        Meta meta;

        /* Load Meta. */
        {
            AMS_UNUSED(path);

            ScopedCodeMount mount(loc, attrs);
            R_TRY(mount.GetResult());
            R_TRY(LoadMeta(std::addressof(meta), loc, mount.GetOverrideStatus(), attrs.platform, false));
            if (out_status != nullptr) {
                *out_status = mount.GetOverrideStatus();
            }
        }

        return GetProgramInfoFromMeta(out, std::addressof(meta));
    }

    Result PinProgram(PinId *out_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status) {
        R_UNLESS(RoManager::GetInstance().Allocate(out_id, loc, override_status), ldr::ResultMaxProcess());
        R_SUCCEED();
    }

    Result UnpinProgram(PinId id) {
        R_UNLESS(RoManager::GetInstance().Free(id), ldr::ResultNotPinned());
        R_SUCCEED();
    }

    Result GetProcessModuleInfo(u32 *out_count, ldr::ModuleInfo *out, size_t max_out_count, os::ProcessId process_id) {
        R_UNLESS(RoManager::GetInstance().GetProcessModuleInfo(out_count, out, max_out_count, process_id), ldr::ResultNotPinned());
        R_SUCCEED();
    }

    Result GetProgramLocationAndOverrideStatusFromPinId(ncm::ProgramLocation *out, cfg::OverrideStatus *out_status, PinId pin_id) {
        R_UNLESS(RoManager::GetInstance().GetProgramLocationAndStatus(out, out_status, pin_id), ldr::ResultNotPinned());
        R_SUCCEED();
    }

}
