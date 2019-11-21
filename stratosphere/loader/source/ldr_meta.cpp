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
#include "ldr_capabilities.hpp"
#include "ldr_content_management.hpp"
#include "ldr_meta.hpp"

namespace ams::ldr {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MetaCacheBufferSize = 0x8000;
        constexpr const char *MetaFilePath = "/main.npdm";

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
            R_UNLESS(size >= min_size,            ResultInvalidMeta());
            R_UNLESS(allowed_start <= start,      ResultInvalidMeta());
            R_UNLESS(start <= allowed_end,        ResultInvalidMeta());
            R_UNLESS(start + size <= allowed_end, ResultInvalidMeta());
            return ResultSuccess();
        }

        Result ValidateNpdm(const Npdm *npdm, size_t size) {
            /* Validate magic. */
            R_UNLESS(npdm->magic == Npdm::Magic, ResultInvalidMeta());

            /* Validate flags. */
            u32 mask = ~0x1F;
            if (hos::GetVersion() < hos::Version_700) {
                /* 7.0.0 added 0x10 as a valid bit to NPDM flags, so before that we only check 0xF. */
                mask = ~0xF;
            }
            R_UNLESS(!(npdm->flags & mask), ResultInvalidMeta());

            /* Validate Acid extents. */
            R_TRY(ValidateSubregion(sizeof(Npdm), size, npdm->acid_offset, npdm->acid_size, sizeof(Acid)));

            /* Validate Aci extends. */
            R_TRY(ValidateSubregion(sizeof(Npdm), size, npdm->aci_offset, npdm->aci_size, sizeof(Aci)));

            return ResultSuccess();
        }

        Result ValidateAcid(const Acid *acid, size_t size) {
            /* Validate magic. */
            R_UNLESS(acid->magic == Acid::Magic, ResultInvalidMeta());

            /* TODO: Check if retail flag is set if not development hardware. */

            /* Validate Fac, Sac, Kac. */
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->fac_offset, acid->fac_size));
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->sac_offset, acid->sac_size));
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->kac_offset, acid->kac_size));

            return ResultSuccess();
        }

        Result ValidateAci(const Aci *aci, size_t size) {
            /* Validate magic. */
            R_UNLESS(aci->magic == Aci::Magic, ResultInvalidMeta());

            /* Validate Fah, Sac, Kac. */
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->fah_offset, aci->fah_size));
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->sac_offset, aci->sac_size));
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->kac_offset, aci->kac_size));

            return ResultSuccess();
        }

        Result LoadMetaFromFile(FILE *f, MetaCache *cache) {
            /* Reset cache. */
            cache->meta = {};

            /* Read from file. */
            size_t npdm_size = 0;
            {
                /* Get file size. */
                fseek(f, 0, SEEK_END);
                npdm_size = ftell(f);
                fseek(f, 0, SEEK_SET);

                /* Read data into cache buffer. */
                R_UNLESS(npdm_size <= MetaCacheBufferSize,            ResultTooLargeMeta());
                R_UNLESS(fread(cache->buffer, npdm_size, 1, f) == 1,  ResultTooLargeMeta());
            }

            /* Ensure size is big enough. */
            R_UNLESS(npdm_size >= sizeof(Npdm), ResultInvalidMeta());

            /* Validate the meta. */
            {
                Meta *meta = &cache->meta;

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
            }

            return ResultSuccess();
        }

    }

    /* API. */
    Result LoadMeta(Meta *out_meta, ncm::ProgramId program_id, const cfg::OverrideStatus &status) {
        FILE *f = nullptr;

        /* Try to load meta from file. */
        R_TRY(OpenCodeFile(f, program_id, status, MetaFilePath));
        {
            ON_SCOPE_EXIT { fclose(f); };
            R_TRY(LoadMetaFromFile(f, &g_meta_cache));
        }

        /* Patch meta. Start by setting all program ids to the current program id. */
        Meta *meta = &g_meta_cache.meta;
        meta->acid->program_id_min = program_id;
        meta->acid->program_id_max = program_id;
        meta->aci->program_id = program_id;

        /* For HBL, we need to copy some information from the base meta. */
        if (status.IsHbl()) {
            if (R_SUCCEEDED(OpenCodeFileFromBaseExefs(f, program_id, status, MetaFilePath))) {
                ON_SCOPE_EXIT { fclose(f); };
                if (R_SUCCEEDED(LoadMetaFromFile(f, &g_original_meta_cache))) {
                    Meta *o_meta = &g_original_meta_cache.meta;

                    /* Fix pool partition. */
                    if (hos::GetVersion() >= hos::Version_500) {
                        meta->acid->flags = (meta->acid->flags & 0xFFFFFFC3) | (o_meta->acid->flags & 0x0000003C);
                    }

                    /* Fix flags. */
                    const u16 program_info_flags = caps::GetProgramInfoFlags(o_meta->aci_kac, o_meta->aci->kac_size);
                    caps::SetProgramInfoFlags(program_info_flags, meta->acid_kac, meta->acid->kac_size);
                    caps::SetProgramInfoFlags(program_info_flags, meta->aci_kac, meta->aci->kac_size);
                }
            }
        }

        /* Set output. */
        g_cached_program_id = program_id;
        g_cached_override_status = status;
        *out_meta = *meta;

        return ResultSuccess();
    }

    Result LoadMetaFromCache(Meta *out_meta, ncm::ProgramId program_id, const cfg::OverrideStatus &status) {
        if (g_cached_program_id != program_id || g_cached_override_status != status) {
            return LoadMeta(out_meta, program_id, status);
        }
        *out_meta = g_meta_cache.meta;
        return ResultSuccess();
    }

    void InvalidateMetaCache() {
        /* Set the cached program id back to zero. */
        g_cached_program_id = {};
    }

}
