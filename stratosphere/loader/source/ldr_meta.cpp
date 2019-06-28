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

#include <stratosphere/cfg.hpp>
#include "ldr_capabilities.hpp"
#include "ldr_content_management.hpp"
#include "ldr_meta.hpp"

namespace sts::ldr {

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
        ncm::TitleId g_cached_title_id;
        MetaCache g_meta_cache;
        MetaCache g_original_meta_cache;

        /* Helpers. */
        Result ValidateSubregion(size_t allowed_start, size_t allowed_end, size_t start, size_t size, size_t min_size = 0) {
            if (!(size >= min_size && allowed_start <= start && start <= allowed_end && start + size <= allowed_end)) {
                return ResultLoaderInvalidMeta;
            }
            return ResultSuccess;
        }

        Result ValidateNpdm(const Npdm *npdm, size_t size) {
            /* Validate magic. */
            if (npdm->magic != Npdm::Magic) {
                return ResultLoaderInvalidMeta;
            }

            /* Validate flags. */
            if (GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
                /* 7.0.0 added 0x10 as a valid bit to NPDM flags. */
                if (npdm->flags & ~0x1F) {
                    return ResultLoaderInvalidMeta;
                }
            } else {
                if (npdm->flags & ~0xF) {
                    return ResultLoaderInvalidMeta;
                }
            }

            /* Validate Acid extents. */
            R_TRY(ValidateSubregion(sizeof(Npdm), size, npdm->acid_offset, npdm->acid_size, sizeof(Acid)));

            /* Validate Aci extends. */
            R_TRY(ValidateSubregion(sizeof(Npdm), size, npdm->aci_offset, npdm->aci_size, sizeof(Aci)));

            return ResultSuccess;
        }

        Result ValidateAcid(const Acid *acid, size_t size) {
            /* Validate magic. */
            if (acid->magic != Acid::Magic) {
                return ResultLoaderInvalidMeta;
            }

            /* TODO: Check if retail flag is set if not development hardware. */

            /* Validate Fac, Sac, Kac. */
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->fac_offset, acid->fac_size));
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->sac_offset, acid->sac_size));
            R_TRY(ValidateSubregion(sizeof(Acid), size, acid->kac_offset, acid->kac_size));

            return ResultSuccess;
        }

        Result ValidateAci(const Aci *aci, size_t size) {
            /* Validate magic. */
            if (aci->magic != Aci::Magic) {
                return ResultLoaderInvalidMeta;
            }

            /* Validate Fah, Sac, Kac. */
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->fah_offset, aci->fah_size));
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->sac_offset, aci->sac_size));
            R_TRY(ValidateSubregion(sizeof(Aci), size, aci->kac_offset, aci->kac_size));

            return ResultSuccess;
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
                if (npdm_size > MetaCacheBufferSize || fread(cache->buffer, npdm_size, 1, f) != 1) {
                    return ResultLoaderTooLargeMeta;
                }
            }

            /* Ensure size is big enough. */
            if (npdm_size < sizeof(Npdm)) {
                return ResultLoaderInvalidMeta;
            }

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

            return ResultSuccess;
        }

    }

    /* API. */
    Result LoadMeta(Meta *out_meta, ncm::TitleId title_id) {
        FILE *f = nullptr;

        /* Try to load meta from file. */
        R_TRY(OpenCodeFile(f, title_id, MetaFilePath));
        {
            ON_SCOPE_EXIT { fclose(f); };
            R_TRY(LoadMetaFromFile(f, &g_meta_cache));
        }

        /* Patch meta. Start by setting all title ids to the current title id. */
        Meta *meta = &g_meta_cache.meta;
        meta->acid->title_id_min = title_id;
        meta->acid->title_id_max = title_id;
        meta->aci->title_id = title_id;

        /* For HBL, we need to copy some information from the base meta. */
        if (cfg::IsHblOverrideKeyHeld(title_id)) {
            if (R_SUCCEEDED(OpenCodeFileFromBaseExefs(f, title_id, MetaFilePath))) {
                ON_SCOPE_EXIT { fclose(f); };
                if (R_SUCCEEDED(LoadMetaFromFile(f, &g_original_meta_cache))) {
                    Meta *o_meta = &g_original_meta_cache.meta;

                    /* Fix pool partition. */
                    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
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
        g_cached_title_id = title_id;
        *out_meta = *meta;

        return ResultSuccess;
    }

    Result LoadMetaFromCache(Meta *out_meta, ncm::TitleId title_id) {
        if (g_cached_title_id != title_id) {
            return LoadMeta(out_meta, title_id);
        }
        *out_meta = g_meta_cache.meta;
        return ResultSuccess;
    }

    void InvalidateMetaCache() {
        /* Set the cached title id back to zero. */
        g_cached_title_id = {};
    }

}
