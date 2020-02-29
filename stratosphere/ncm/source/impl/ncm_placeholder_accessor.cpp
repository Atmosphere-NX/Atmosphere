/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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

#include "ncm_placeholder_accessor.hpp"
#include "../ncm_fs.hpp"
#include "../ncm_utils.hpp"
#include "../ncm_make_path.hpp"
#include "../ncm_path_utils.hpp"

namespace ams::ncm::impl {

    namespace {

        ALWAYS_INLINE Result ConvertNotFoundResult(Result r) {
            R_TRY_CATCH(r) {
                R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultPlaceHolderNotFound())
            } R_END_TRY_CATCH;
            return ResultSuccess();
        }

    }

    Result PlaceHolderAccessor::Open(FILE** out_handle, PlaceHolderId placeholder_id) {
        R_UNLESS(!this->LoadFromCache(out_handle, placeholder_id), ResultSuccess());
        char placeholder_path[FS_MAX_PATH] = {0};
        this->MakePath(placeholder_path, placeholder_id);

        FILE *f = nullptr;
        R_TRY(fs::OpenFile(&f, placeholder_path, FsOpenMode_Write));

        *out_handle = f;
        return ResultSuccess();
    }

    bool PlaceHolderAccessor::LoadFromCache(FILE** out_handle, PlaceHolderId placeholder_id) {
        std::scoped_lock lk(this->cache_mutex);
        CacheEntry *entry = this->FindInCache(placeholder_id);
        if (!entry) {
            return false;
        }
        *out_handle = entry->handle;
        entry->id = InvalidPlaceHolderId;
        entry->handle = nullptr;
        return true;
    }

    PlaceHolderAccessor::CacheEntry *PlaceHolderAccessor::FindInCache(PlaceHolderId placeholder_id) {
        for (size_t i = 0; i < MaxCaches; i++) {
            if (placeholder_id == this->caches[i].id) {
                return &this->caches[i];
            }
        }
        return nullptr;
    }

    PlaceHolderAccessor::CacheEntry *PlaceHolderAccessor::GetFreeEntry() {
        /* Try to find an already free entry. */
        if (CacheEntry *entry = this->FindInCache(InvalidPlaceHolderId); entry != nullptr) {
            return entry;
        }

        /* Get the oldest entry. */
        CacheEntry *entry = &this->caches[0];
        for (size_t i = 1; i < MaxCaches; i++) {
            if (entry->counter > this->caches[i].counter) {
                entry = &this->caches[i];
            }
        }
        this->Invalidate(entry);
        return entry;
    }

    void PlaceHolderAccessor::StoreToCache(FILE *handle, PlaceHolderId placeholder_id) {
        std::scoped_lock lk(this->cache_mutex);
        CacheEntry *entry = this->GetFreeEntry();
        entry->id = placeholder_id;
        entry->handle = handle;
        entry->counter = this->cur_counter++;
    }

    void PlaceHolderAccessor::Invalidate(CacheEntry *entry) {
        if (entry != nullptr) {
            if (entry->handle != nullptr) {
                fflush(entry->handle);
                fclose(entry->handle);
                entry->handle = nullptr;
            }
            entry->id = InvalidPlaceHolderId;
        }
    }

    void PlaceHolderAccessor::Initialize(char *root, MakePlaceHolderPathFunc path_func, bool delay_flush) {
        this->root_path = root;
        this->make_placeholder_path_func = path_func;
        this->delay_flush = delay_flush;
    }

    unsigned int PlaceHolderAccessor::GetDirectoryDepth() {
        if (this->make_placeholder_path_func == static_cast<MakePlaceHolderPathFunc>(path::MakePlaceHolderPathFlat)) {
            return 1;
        } else if (this->make_placeholder_path_func == static_cast<MakePlaceHolderPathFunc>(path::MakePlaceHolderPathHashByteLayered)) {
            return 2;
        } else {
            AMS_ABORT();
        }
    }

    void PlaceHolderAccessor::GetPath(char *placeholder_path_out, PlaceHolderId placeholder_id) {
        std::scoped_lock lock(this->cache_mutex);
        CacheEntry *entry = this->FindInCache(placeholder_id);
        this->Invalidate(entry);
        this->MakePath(placeholder_path_out, placeholder_id);
    }

    Result PlaceHolderAccessor::Create(PlaceHolderId placeholder_id, size_t size) {
        char placeholder_path[FS_MAX_PATH] = {0};

        this->EnsureRecursively(placeholder_id);
        this->GetPath(placeholder_path, placeholder_id);

        R_TRY_CATCH(fsdevCreateFile(placeholder_path, size, FsCreateOption_BigFile)) {
            R_CONVERT(ams::fs::ResultPathAlreadyExists, ncm::ResultPlaceHolderAlreadyExists())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result PlaceHolderAccessor::Delete(PlaceHolderId placeholder_id) {
        char placeholder_path[FS_MAX_PATH] = {0};

        this->GetPath(placeholder_path, placeholder_id);
        R_UNLESS(std::remove(placeholder_path) == 0, ConvertNotFoundResult(fsdevGetLastResult()));

        return ResultSuccess();
    }

    Result PlaceHolderAccessor::Write(PlaceHolderId placeholder_id, size_t offset, const void *buffer, size_t size) {
        FILE *f = nullptr;

        R_TRY(ConvertNotFoundResult(this->Open(&f, placeholder_id)));
        ON_SCOPE_EXIT { this->StoreToCache(f, placeholder_id); };

        R_TRY(fs::WriteFile(f, offset, buffer, size, this->delay_flush ? ams::fs::WriteOption::Flush : ams::fs::WriteOption::None));
        return ResultSuccess();
    }

    Result PlaceHolderAccessor::SetSize(PlaceHolderId placeholder_id, size_t size) {
        char placeholder_path[FS_MAX_PATH] = {0};
        this->MakePath(placeholder_path, placeholder_id);
        R_UNLESS(truncate(placeholder_path, size) != -1, ConvertNotFoundResult(fsdevGetLastResult()));

        return ResultSuccess();
    }

    Result PlaceHolderAccessor::GetSize(bool *found_in_cache, size_t *out_size, PlaceHolderId placeholder_id) {
        FILE *f = NULL;

        *found_in_cache = false;

        /* Set the scope for the scoped_lock. */
        {
            std::scoped_lock lock(this->cache_mutex);
            
            /* If the placeholder id is invalid, return success early. */
            R_UNLESS(placeholder_id != InvalidPlaceHolderId, ResultSuccess());

            CacheEntry *cache_entry = this->FindInCache(placeholder_id);

            /* If there is no entry in the cache, return success early. */
            R_UNLESS(cache_entry != nullptr, ResultSuccess());

            cache_entry->id = InvalidPlaceHolderId;
            f = cache_entry->handle;
        }

        this->StoreToCache(f, placeholder_id);
        
        R_UNLESS(fseek(f, 0L, SEEK_END) == 0, fsdevGetLastResult());
        size_t size = ftell(f);
        R_UNLESS(fseek(f, 0L, SEEK_SET) == 0, fsdevGetLastResult());

        *found_in_cache = true;
        *out_size = size;
        return ResultSuccess();
    }

    Result PlaceHolderAccessor::EnsureRecursively(PlaceHolderId placeholder_id) {
        char placeholder_path[FS_MAX_PATH] = {0};
        this->MakePath(placeholder_path, placeholder_id);
        R_TRY(fs::EnsureParentDirectoryRecursively(placeholder_path));
        return ResultSuccess();
    }

    void PlaceHolderAccessor::InvalidateAll() {
        for (auto &entry : this->caches) {
            if (entry.id != InvalidPlaceHolderId) {
                this->Invalidate(&entry);
            }
        }
    }

}
