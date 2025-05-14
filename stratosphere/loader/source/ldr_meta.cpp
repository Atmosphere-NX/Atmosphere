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
#include "ldr_meta.hpp"

namespace ams::ldr {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MetaCacheBufferSize = 0x8000;
        constexpr inline const char AtmosphereMetaPath[] = ENCODE_ATMOSPHERE_CODE_PATH("/main.npdm");
        constexpr inline const char SdOrBaseMetaPath[]   = ENCODE_SD_OR_CODE_PATH("/main.npdm");
        constexpr inline const char BaseMetaPath[]       = ENCODE_CODE_PATH("/main.npdm");

        /* Types. */
        struct MetaCache {
            Meta meta;
            u8 buffer[MetaCacheBufferSize];
        };

        /* Global storage. */
        ncm::ProgramId g_cached_program_id;
        cfg::OverrideStatus g_cached_override_status;
        MetaCache g_meta_cache;
        MetaCache g_original_meta_cache;

        /* Helpers. */
        Result ValidateSubregion(size_t allowed_start, size_t allowed_end, size_t start, size_t size, size_t min_size = 0) {
            R_UNLESS(size >= min_size,            ldr::ResultInvalidMeta());
            R_UNLESS(allowed_start <= start,      ldr::ResultInvalidMeta());
            R_UNLESS(start <= allowed_end,        ldr::ResultInvalidMeta());
            R_UNLESS(start + size <= allowed_end, ldr::ResultInvalidMeta());
            R_SUCCEED();
        }

        Result ValidateNpdm(const Npdm *npdm, size_t size) {
            /* Validate magic. */
            R_UNLESS(npdm->magic == Npdm::Magic, ldr::ResultInvalidMeta());

            /* Validate flags. */
            constexpr u32 InvalidMetaFlagMask = 0x80000000;
            R_UNLESS(!(npdm->flags & InvalidMetaFlagMask), ldr::ResultInvalidMeta());

            /* Validate Acid extents. */
            R_TRY(ValidateSubregion(sizeof(Npdm), size, npdm->acid_offset, npdm->acid_size, sizeof(Acid)));

            /* Validate Aci extends. */
            R_TRY(ValidateSubregion(sizeof(Npdm), size, npdm->aci_offset, npdm->aci_size, sizeof(Aci)));

            R_SUCCEED();
        }

        Result ValidateAcid(const Acid *acid, size_t size) {
            /* Validate magic. */
            R_UNLESS(acid->magic == Acid::Magic, ldr::ResultInvalidMeta());

            /* Validate that the acid is for production if not development. */
            if (!IsDevelopmentForAcidProductionCheck()) {
                R_UNLESS((acid->flags & Acid::AcidFlag_Production) != 0, ldr::ResultInvalidMeta());
            }

            /* Validate that the acid version is correct. */
            constexpr u8 SupportedSdkMajorVersion = ams::svc::ConvertToSdkMajorVersion(ams::svc::SupportedKernelMajorVersion);
            if (acid->unknown_209 < SupportedSdkMajorVersion) {
                R_UNLESS(acid->version == 0,     ldr::ResultInvalidMeta());
                R_UNLESS(acid->unknown_209 == 0, ldr::ResultInvalidMeta());
            }

            /* Validate Fac, Sac, Kac. */
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->fac_offset, acid->fac_size));
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->sac_offset, acid->sac_size));
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->kac_offset, acid->kac_size));

            R_SUCCEED();
        }

        Result ValidateAci(const Aci *aci, size_t size) {
            /* Validate magic. */
            R_UNLESS(aci->magic == Aci::Magic, ldr::ResultInvalidMeta());

            /* Validate Fah, Sac, Kac. */
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->fah_offset, aci->fah_size));
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->sac_offset, aci->sac_size));
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->kac_offset, aci->kac_size));

            R_SUCCEED();
        }

        const u8 *GetAcidSignatureModulus(ncm::ContentMetaPlatform platform, u8 key_generation, bool unk_unused) {
            return fssystem::GetAcidSignatureKeyModulus(platform, !IsDevelopmentForAcidSignatureCheck(), key_generation, unk_unused);
        }

        size_t GetAcidSignatureModulusSize(ncm::ContentMetaPlatform platform, bool unk_unused) {
            return fssystem::GetAcidSignatureKeyModulusSize(platform, unk_unused);
        }

        Result ValidateAcidSignature(Meta *meta, ncm::ContentMetaPlatform platform, bool unk_unused) {
            /* Loader did not check signatures prior to 10.0.0. */
            if (hos::GetVersion() < hos::Version_10_0_0) {
                meta->check_verification_data = false;
                R_SUCCEED();
            }

            /* Get the signature key generation. */
            const auto signature_key_generation = meta->npdm->signature_key_generation;
            R_UNLESS(fssystem::IsValidSignatureKeyGeneration(platform, signature_key_generation), ldr::ResultInvalidMeta());

            /* Verify the signature. */
            const u8 *sig         = meta->acid->signature;
            const size_t sig_size = sizeof(meta->acid->signature);
            const u8 *mod         = GetAcidSignatureModulus(platform, signature_key_generation, unk_unused);
            const size_t mod_size = GetAcidSignatureModulusSize(platform, unk_unused);
            const u8 *exp         = fssystem::GetAcidSignatureKeyPublicExponent();
            const size_t exp_size = fssystem::AcidSignatureKeyPublicExponentSize;
            const u8 *msg         = meta->acid->modulus;
            const size_t msg_size = meta->acid->size;
            const bool is_signature_valid = crypto::VerifyRsa2048PssSha256(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
            R_UNLESS(is_signature_valid || !IsEnabledProgramVerification(), ldr::ResultInvalidAcidSignature());

            meta->check_verification_data = is_signature_valid;
            R_SUCCEED();
        }

        Result LoadMetaFromFile(fs::FileHandle file, MetaCache *cache) {
            /* Reset cache. */
            cache->meta = {};

            /* Read from file. */
            s64 npdm_size = 0;
            {
                /* Get file size. */
                R_TRY(fs::GetFileSize(std::addressof(npdm_size), file));

                /* Read data into cache buffer. */
                R_UNLESS(npdm_size <= static_cast<s64>(MetaCacheBufferSize), ldr::ResultMetaOverflow());
                R_TRY(fs::ReadFile(file, 0, cache->buffer, npdm_size));
            }

            /* Ensure size is big enough. */
            R_UNLESS(npdm_size >= static_cast<s64>(sizeof(Npdm)), ldr::ResultInvalidMeta());

            /* Validate the meta. */
            {
                Meta *meta = std::addressof(cache->meta);

                Npdm *npdm = reinterpret_cast<Npdm *>(cache->buffer);
                R_TRY(ValidateNpdm(npdm, npdm_size));

                Acid *acid = reinterpret_cast<Acid *>(cache->buffer + npdm->acid_offset);
                Aci *aci = reinterpret_cast<Aci *>(cache->buffer + npdm->aci_offset);
                R_TRY(ValidateAcid(acid, npdm->acid_size));
                R_TRY(ValidateAci(aci, npdm->aci_size));

                /* Set Meta members. */
                meta->npdm = npdm;
                meta->acid = acid;
                meta->aci = aci;

                meta->acid_fac = reinterpret_cast<u8 *>(acid) + acid->fac_offset;
                meta->acid_sac = reinterpret_cast<u8 *>(acid) + acid->sac_offset;
                meta->acid_kac = reinterpret_cast<u8 *>(acid) + acid->kac_offset;

                meta->aci_fah = reinterpret_cast<u8 *>(aci) + aci->fah_offset;
                meta->aci_sac = reinterpret_cast<u8 *>(aci) + aci->sac_offset;
                meta->aci_kac = reinterpret_cast<u8 *>(aci) + aci->kac_offset;

                meta->modulus   = acid->modulus;
            }

            R_SUCCEED();
        }

    }

    /* API. */
    Result LoadMeta(Meta *out_meta, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status, ncm::ContentMetaPlatform platform, bool unk_unused) {
        /* Set the cached program id back to zero. */
        g_cached_program_id = {};

        /* Try to load meta from file. */
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), AtmosphereMetaPath, fs::OpenMode_Read));
        {
            ON_SCOPE_EXIT { fs::CloseFile(file); };
            R_TRY(LoadMetaFromFile(file, std::addressof(g_meta_cache)));
        }

        /* Patch meta. Start by setting all program ids to the current program id. */
        Meta *meta = std::addressof(g_meta_cache.meta);
        meta->acid->program_id_min = loc.program_id;
        meta->acid->program_id_max = loc.program_id;
        meta->aci->program_id      = loc.program_id;

        /* For HBL, we need to copy some information from the base meta. */
        if (status.IsHbl()) {
            if (R_SUCCEEDED(fs::OpenFile(std::addressof(file), SdOrBaseMetaPath, fs::OpenMode_Read))) {
                ON_SCOPE_EXIT { fs::CloseFile(file); };


                if (R_SUCCEEDED(LoadMetaFromFile(file, std::addressof(g_original_meta_cache)))) {
                    Meta *o_meta = std::addressof(g_original_meta_cache.meta);

                    /* Fix pool partition. */
                    if (hos::GetVersion() >= hos::Version_5_0_0) {
                        meta->acid->flags = (meta->acid->flags & 0xFFFFFFC3) | (o_meta->acid->flags & 0x0000003C);
                    }

                    /* Fix flags. */
                    const u16 program_info_flags = MakeProgramInfoFlag(static_cast<const util::BitPack32 *>(o_meta->aci_kac), o_meta->aci->kac_size / sizeof(util::BitPack32));
                    UpdateProgramInfoFlag(program_info_flags, static_cast<util::BitPack32 *>(meta->acid_kac), meta->acid->kac_size / sizeof(util::BitPack32));
                    UpdateProgramInfoFlag(program_info_flags, static_cast<util::BitPack32 *>(meta->aci_kac),  meta->aci->kac_size  / sizeof(util::BitPack32));
                }
            }

            /* Perform address space override. */
            if (status.HasOverrideAddressSpace()) {
                /* Clear the existing address space. */
                meta->npdm->flags &= ~Npdm::MetaFlag_AddressSpaceTypeMask;

                /* Set the new address space flag. */
                switch (status.GetOverrideAddressSpaceFlags()) {
                    case cfg::impl::OverrideStatusFlag_AddressSpace32Bit:             meta->npdm->flags |= (Npdm::AddressSpaceType_32Bit)             << Npdm::MetaFlag_AddressSpaceTypeShift; break;
                    case cfg::impl::OverrideStatusFlag_AddressSpace64BitDeprecated:   meta->npdm->flags |= (Npdm::AddressSpaceType_64BitDeprecated)   << Npdm::MetaFlag_AddressSpaceTypeShift; break;
                    case cfg::impl::OverrideStatusFlag_AddressSpace32BitWithoutAlias: meta->npdm->flags |= (Npdm::AddressSpaceType_32BitWithoutAlias) << Npdm::MetaFlag_AddressSpaceTypeShift; break;
                    case cfg::impl::OverrideStatusFlag_AddressSpace64Bit:             meta->npdm->flags |= (Npdm::AddressSpaceType_64Bit)             << Npdm::MetaFlag_AddressSpaceTypeShift; break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            /* When hbl is applet, adjust main thread priority. */
            if ((MakeProgramInfoFlag(static_cast<const util::BitPack32 *>(meta->aci_kac), meta->aci->kac_size / sizeof(util::BitPack32)) & ProgramInfoFlag_ApplicationTypeMask) == ProgramInfoFlag_Applet) {
                constexpr auto HblMainThreadPriorityApplication = 44;
                constexpr auto HblMainThreadPriorityApplet      = 40;
                if (meta->npdm->main_thread_priority == HblMainThreadPriorityApplication) {
                    meta->npdm->main_thread_priority = HblMainThreadPriorityApplet;
                }
            }

            /* Fix the debug capabilities, to prevent needing a hbl recompilation. */
            FixDebugCapabilityForHbl(static_cast<util::BitPack32 *>(meta->acid_kac), meta->acid->kac_size / sizeof(util::BitPack32));
            FixDebugCapabilityForHbl(static_cast<util::BitPack32 *>(meta->aci_kac),  meta->aci->kac_size  / sizeof(util::BitPack32));
        } else if (hos::GetVersion() >= hos::Version_10_0_0) {
            /* If storage id is none, there is no base code filesystem, and thus it is impossible for us to validate. */
            /* However, if we're an application, we are guaranteed a base code filesystem. */
            if (static_cast<ncm::StorageId>(loc.storage_id) != ncm::StorageId::None || ncm::IsApplicationId(loc.program_id)) {
                R_TRY(fs::OpenFile(std::addressof(file), BaseMetaPath, fs::OpenMode_Read));
                ON_SCOPE_EXIT { fs::CloseFile(file); };
                R_TRY(LoadMetaFromFile(file, std::addressof(g_original_meta_cache)));
                R_TRY(ValidateAcidSignature(std::addressof(g_original_meta_cache.meta), platform, unk_unused));
                meta->modulus                 = g_original_meta_cache.meta.modulus;
                meta->check_verification_data = g_original_meta_cache.meta.check_verification_data;
            }
        }

        /* Pre-process the capabilities. */
        /* This is used to e.g. avoid passing memory region descriptor to older kernels. */
        PreProcessCapability(static_cast<util::BitPack32 *>(meta->acid_kac), meta->acid->kac_size / sizeof(util::BitPack32));
        PreProcessCapability(static_cast<util::BitPack32 *>(meta->aci_kac),  meta->aci->kac_size  / sizeof(util::BitPack32));

        /* Set output. */
        g_cached_program_id = loc.program_id;
        g_cached_override_status = status;
        *out_meta = *meta;

        R_SUCCEED();
    }

    Result LoadMetaFromCache(Meta *out_meta, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status, ncm::ContentMetaPlatform platform) {
        if (g_cached_program_id != loc.program_id || g_cached_override_status != status) {
            R_RETURN(LoadMeta(out_meta, loc, status, platform, false));
        }
        *out_meta = g_meta_cache.meta;
        R_SUCCEED();
    }

    void InvalidateMetaCache() {
        /* Set the cached program id back to zero. */
        g_cached_program_id = {};
    }

}
