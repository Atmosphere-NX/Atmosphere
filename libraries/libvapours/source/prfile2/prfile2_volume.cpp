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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "prfile2_volume_set.hpp"

namespace ams::prfile2::vol {

    /* Global volume context object. */
    constinit VolumeContext g_vol_set;

    namespace {

        constexpr inline u32 CharacterCheckDisable = 0x10000;
        constexpr inline u32 CharacterCheckEnable  = 0x20000;

        constexpr inline u32 CharacterCheckMask    = CharacterCheckDisable | CharacterCheckEnable;

        constexpr inline u32 VolumeSetConfigMask = 0x5FFFFFFF;

        VolumeContext *GetVolumeContextById(u64 context_id) {
            /* Get the volume set. */
            auto &vol_set = GetVolumeSet();

            /* Acquire exclusive access to the volume set. */
            ScopedCriticalSection lk(vol_set.critical_section);

            /* Find a matching context. */
            for (auto *ctx = vol_set.used_context_head; ctx != nullptr; ctx = ctx->next_used_context) {
                if (ctx->context_id == context_id) {
                    return ctx;
                }
            }

            return nullptr;
        }

    }

    pf::Error Initialize(u32 config, void *param) {
        /* Check the input config. */
        if ((config & ~CharacterCheckMask) != 0) {
            return pf::Error_InvalidParameter;
        }
        if ((config & CharacterCheckMask) == CharacterCheckMask) {
            return pf::Error_InvalidParameter;
        }

        /* Get the volume set. */
        auto &vol_set = GetVolumeSet();

        /* Clear the default volume context. */
        std::memset(std::addressof(vol_set.default_context), 0, sizeof(VolumeContext));
        vol_set.default_context.volume_id = 0;

        /* Setup the context lists. */
        vol_set.used_context_head = nullptr;
        vol_set.used_context_tail = nullptr;
        vol_set.free_context_head = vol_set.contexts;
        for (auto i = 0; i < MaxVolumes - 1; ++i) {
            vol_set.contexts[i].next_free_context = std::addressof(vol_set.contexts[i + 1]);
        }
        vol_set.contexts[MaxVolumes - 1].next_free_context = nullptr;

        /* Set the setting. */
        vol_set.setting = 1;

        /* Set the config. */
        if ((config & CharacterCheckEnable) != 0) {
            vol_set.config |= CharacterCheckDisable;
        } else {
            vol_set.config &= ~CharacterCheckDisable;
        }
        vol_set.config &= VolumeSetConfigMask;

        /* Clear number of attached drives/volumes. */
        vol_set.num_attached_drives = 0;
        vol_set.num_mounted_volumes = 0;

        /* Set the parameter. */
        vol_set.param = param;

        /* Set the codeset. */
        /* TODO */

        /* Clear the volumes. */
        for (auto &volume : vol_set.volumes) {
            std::memset(std::addressof(volume), 0, sizeof(volume));
        }

        /* Initialize the volume set critical section. */
        InitializeCriticalSection(std::addressof(vol_set.critical_section));

        /* NOTE: Here "InitLockFile()" is called, but this doesn't seem used so far. TODO: Add if used? */

        /* Mark initialized. */
        vol_set.initialized = true;

        return pf::Error_Ok;
    }

    pf::Error Attach(pf::DriveTable *drive_table) {
        AMS_UNUSED(drive_table);
        AMS_ABORT("vol::Attach");
    }

    VolumeContext *RegisterContext(u64 *out_context_id) {
        /* Get the current context id. */
        u64 context_id = 0;
        system::GetCurrentContextId(std::addressof(context_id));
        if (out_context_id != nullptr) {
            *out_context_id = context_id;
        }

        /* Get the volume set. */
        auto &vol_set = GetVolumeSet();

        /* Acquire exclusive access to the volume set. */
        ScopedCriticalSection lk(vol_set.critical_section);

        /* Get the volume context by ID. If we already have a context, return it. */
        if (VolumeContext *match = GetVolumeContextById(context_id); match != nullptr) {
            return match;
        }

        /* Try to find a free context in the list. */
        VolumeContext *ctx = vol_set.free_context_head;
        if (ctx == nullptr) {
            vol_set.default_context.last_error = pf::Error_InternalError;
            return nullptr;
        }

        /* Update the free lists. */
        vol_set.free_context_head = ctx->next_free_context;
        if (VolumeContext *next = vol_set.used_context_head; next != nullptr) {
            next->prev_used_context   = ctx;
            ctx->next_used_context    = next;
            ctx->prev_used_context    = nullptr;
            vol_set.used_context_head = ctx;
        } else {
            ctx->next_used_context    = nullptr;
            ctx->prev_used_context    = nullptr;
            vol_set.used_context_head = ctx;
            vol_set.used_context_tail = ctx;
        }

        /* Set the context's fields. */
        ctx->context_id = context_id;
        ctx->last_error = pf::Error_Ok;
        for (auto i = 0; i < MaxVolumes; ++i) {
            ctx->last_driver_error[i] = pf::Error_Ok;
            ctx->last_unk_error[i]    = pf::Error_Ok;
        }

        /* Copy from the default context. */
        const auto volume_id = vol_set.default_context.volume_id;
        ctx->volume_id = volume_id;
        ctx->dir_entries[volume_id] = vol_set.default_context.dir_entries[volume_id];

        return ctx;
    }

    pf::Error UnregisterContext() {
        /* Get the current context id. */
        u64 context_id = 0;
        system::GetCurrentContextId(std::addressof(context_id));

        /* Get the volume set. */
        auto &vol_set = GetVolumeSet();

        /* Acquire exclusive access to the volume set. */
        ScopedCriticalSection lk(vol_set.critical_section);

        /* Get the volume context by ID. */
        VolumeContext *ctx = GetVolumeContextById(context_id);
        if (ctx == nullptr) {
            vol_set.default_context.last_error = pf::Error_InternalError;
            return pf::Error_InternalError;
        }

        /* Update the lists. */
        auto *prev_used = ctx->prev_used_context;
        auto *next_used = ctx->next_used_context;
        if (prev_used != nullptr) {
            if (next_used != nullptr) {
                prev_used->next_used_context = next_used;
                next_used->prev_used_context = prev_used;
            } else {
                prev_used->next_used_context = nullptr;
                vol_set.used_context_tail    = prev_used;
            }
        } else if (next_used != nullptr) {
            next_used->prev_used_context = nullptr;
            vol_set.used_context_head    = next_used;
        } else {
            vol_set.used_context_head = nullptr;
            vol_set.used_context_tail = nullptr;
        }

        ctx->next_used_context = nullptr;
        ctx->next_free_context = vol_set.free_context_head;
        vol_set.free_context_head = ctx;

        return pf::Error_Ok;
    }

}
