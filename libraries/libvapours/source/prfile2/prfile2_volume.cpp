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

        VolumeContext *GetCurrentVolumeContext(u64 *out_context_id) {
            /* Get the volume set. */
            auto &vol_set = GetVolumeSet();

            /* Acquire exclusive access to the volume set. */
            ScopedCriticalSection lk(vol_set.critical_section);

            /* Get the current context id. */
            u64 context_id = 0;
            system::GetCurrentContextId(std::addressof(context_id));
            if (out_context_id != nullptr) {
                *out_context_id = context_id;
            }

            if (auto *ctx = GetVolumeContextById(context_id); ctx != nullptr) {
                return ctx;
            } else {
                return std::addressof(vol_set.default_context);
            }
        }

        bool IsValidDriveCharacter(pf::DriveCharacter drive_char) {
            return static_cast<u8>((drive_char & 0xDF) - 'A') < 26;
        }

        Volume *GetVolumeByDriveCharacter(pf::DriveCharacter drive_char) {
            /* Get the volume set. */
            auto &vol_set = GetVolumeSet();

            /* Calculate the volume index. */
            const auto index = std::toupper(static_cast<unsigned char>(drive_char)) - 'A';

            /* Acquire exclusive access to the volume set. */
            ScopedCriticalSection lk(vol_set.critical_section);

            if (index < MaxVolumes) {
                return std::addressof(vol_set.volumes[index]);
            } else {
                return nullptr;
            }
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
        /* Get the volume set. */
        auto &vol_set = GetVolumeSet();

        /* Get the volume context for error tracking. */
        u64 context_id = 0;
        auto *vol_ctx = GetCurrentVolumeContext(std::addressof(context_id));

        auto SetLastErrorAndReturn = [&] ALWAYS_INLINE_LAMBDA (pf::Error err) -> pf::Error { vol_ctx->last_error = err; return err; };

        /* Check the drive table. */
        if (drive_table == nullptr) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }

        /* Clear the drive table's character/status. */
        const auto drive_char = drive_table->drive_char;
        drive_table->drive_char = 0;
        drive_table->status     = 0;

        /* Check that we can attach. */
        if (vol_set.num_attached_drives >= MaxVolumes) {
            return SetLastErrorAndReturn(pf::Error_TooManyVolumesAttached);
        }

        /* Check the cache setting. */
        auto *cache_setting = drive_table->cache;
        if (cache_setting == nullptr) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (cache_setting->fat_buf_size > MaximumFatBufferSize) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (cache_setting->data_buf_size > MaximumDataBufferSize) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (cache_setting->num_fat_pages < MinimumFatPages) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (cache_setting->num_data_pages < MinimumDataPages) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (cache_setting->pages == nullptr) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (cache_setting->buffers == nullptr) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (!util::IsAligned(reinterpret_cast<uintptr_t>(cache_setting->pages), alignof(u32))) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if (!util::IsAligned(reinterpret_cast<uintptr_t>(cache_setting->buffers), alignof(u32))) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }

        /* Adjust the cache setting. */
        cache_setting->fat_buf_size  = std::max<u32>(cache_setting->fat_buf_size, MinimumFatBufferSize);
        cache_setting->data_buf_size = std::max<u32>(cache_setting->data_buf_size, MinimumDataBufferSize);

        /* Check the adjusted setting. */
        if (cache_setting->fat_buf_size > cache_setting->num_fat_pages) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }
        if ((cache_setting->num_data_pages / cache_setting->data_buf_size) < MinimumDataPages) {
            return SetLastErrorAndReturn(pf::Error_InvalidParameter);
        }

        /* Validate the drive character. */
        if (drive_char != 0) {
            if (!IsValidDriveCharacter(drive_char)) {
                return SetLastErrorAndReturn(pf::Error_InvalidParameter);
            }

            if (auto *vol = GetVolumeByDriveCharacter(drive_char); vol == nullptr || vol->IsAttached()) {
                return SetLastErrorAndReturn(pf::Error_InvalidVolumeLabel);
            }
        }

        /* Perform the bulk of the attach. */
        Volume *vol = nullptr;
        {
            /* Lock the volume set. */
            ScopedCriticalSection lk(vol_set.critical_section);

            /* Find a free volume. */
            for (auto &v : vol_set.volumes) {
                if (!v.IsAttached()) {
                    vol = std::addressof(v);
                    break;
                }
            }
            if (vol == nullptr) {
                return SetLastErrorAndReturn(pf::Error_TooManyVolumesAttached);
            }
            const auto vol_id = vol - vol_set.volumes;

            /* Clear the volume. */
            std::memset(vol, 0, sizeof(*vol));

            /* Initialize the volume. */
            vol->num_free_clusters  = InvalidCluster;
            vol->num_free_clusters_ = InvalidCluster;
            vol->last_free_cluster  = InvalidCluster;
            vol->partition_handle   = drive_table->partition_handle;
            InitializeCriticalSection(std::addressof(vol->critical_section));
            vol->drive_char         = 'A' + vol_id;

            /* Setup directory tail. */
            vol->tail_entry.tracker_size = util::size(vol->tail_entry.tracker_buf);
            vol->tail_entry.tracker_bits = vol->tail_entry.tracker_buf;

            /* Initialize driver for volume. */
            /* TODO: drv::Initialize(vol); + error checking */

            /* Setup the cache. */
            /* TODO: cache::SetCache(vol, drive_table->cache->pages, drive_table->cache->buffers, drive_table->cache->num_fat_pages, drive_table->cache->num_data_pages); */
            /* TODO: cache::SetFatBufferSize(vol, drive_table->cache->fat_buf_size); */
            /* TODO: cache::SetDataBufferSize(vol, drive_table->cache->data_buf_size); */

            /* Set flags. */
            vol->SetAttached(true);
            vol->SetFlag12(true);

            /* Update the drive table. */
            drive_table->SetAttached(true);
            drive_table->drive_char = vol->drive_char;

            /* Update the number of attached drives. */
            if ((vol_set.num_attached_drives++) == 0) {
                vol_set.default_context.volume_id = vol_id;
            }
        }

        /* Acquire exclusive access to the volume set. */
        ScopedCriticalSection lk(vol_set.critical_section);

        /* Associate the volume to our context while we operate on it. */
        vol->context    = vol_ctx;
        vol->context_id = context_id;
        ON_SCOPE_EXIT { vol->context_id = 0; };

        /* TODO: Copy volume root dir entry to all contexts. */

        /* TODO: Clear tracking fields at the end of the volume. */

        /* Perform mount as appropriate. */
        const auto check_mount_err = /* TODO vol::CheckMediaInsertForAttachMount(vol) */ pf::Error_Ok;
        const bool inserted        = /* TODO: drv::IsInserted(vol) */ false;
        if (check_mount_err != pf::Error_Ok) {
            if (inserted) {
                drive_table->SetDiskInserted(true);
            }
            vol_ctx->last_error = check_mount_err;
        } else if (inserted) {
            drive_table->SetDiskInserted(true);
            if (auto mount_err = /* TODO vol::MountImpl(vol, 0x1B, false) */pf::Error_InternalError; mount_err != pf::Error_Ok) {
                vol_ctx->last_error = mount_err;
            } else {
                drive_table->SetMounted(true);
            }
        }

        return pf::Error_Ok;
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
