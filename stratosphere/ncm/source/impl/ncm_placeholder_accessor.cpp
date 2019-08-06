/*
 * Copyright (c) 2019 Adubbz
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

#include "../debug.hpp"

namespace sts::ncm::impl {

    unsigned int PlaceHolderAccessor::GetDirectoryDepth() {
        if (this->make_placeholder_path_func == static_cast<MakePlaceHolderPathFunc>(path::MakePlaceHolderPathFlat)) {
            return 1;
        } else if (this->make_placeholder_path_func == static_cast<MakePlaceHolderPathFunc>(path::MakePlaceHolderPathHashByteLayered)) {
            return 2;
        }

        std::abort();
    }

    void PlaceHolderAccessor::GetPlaceHolderPathUncached(char* placeholder_path_out, PlaceHolderId placeholder_id) {
        std::scoped_lock<HosMutex> lock(this->cache_mutex);

        if (placeholder_id != InvalidUuid) {
            CacheEntry* found_cache = NULL;
            
            for (size_t i = 0; i < PlaceHolderAccessor::MaxCaches; i++) {
                CacheEntry* cache = &this->caches[i];

                if (placeholder_id == cache->id) {
                    found_cache = cache;
                    break;
                }
            }

            if (found_cache) {
                /* Flush and close */
                fflush(found_cache->handle);
                fclose(found_cache->handle);
                std::fill(found_cache->id.uuid, found_cache->id.uuid + sizeof(PlaceHolderId), 0);
            }
        }

        this->GetPlaceHolderPath(placeholder_path_out, placeholder_id);
    }

    Result PlaceHolderAccessor::Create(PlaceHolderId placeholder_id, size_t size) {
        char placeholder_path[FS_MAX_PATH] = {0};

        this->EnsureRecursively(placeholder_id);
        this->GetPlaceHolderPathUncached(placeholder_path, placeholder_id);

        debug::DebugLog("Creating %s\n", placeholder_path);
        R_TRY_CATCH(fsdevCreateFile(placeholder_path, size, FS_CREATE_BIG_FILE)) {
            R_CATCH(ResultFsPathAlreadyExists) {
                return ResultNcmPlaceHolderAlreadyExists;
            }
        } R_END_TRY_CATCH;

        return ResultSuccess;
    }

    Result PlaceHolderAccessor::Delete(PlaceHolderId placeholder_id) {
        char placeholder_path[FS_MAX_PATH] = {0};

        this->GetPlaceHolderPathUncached(placeholder_path, placeholder_id);

        debug::DebugLog("Deleting %s\n", placeholder_path);
        if (std::remove(placeholder_path) != 0) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ResultFsPathNotFound) {
                    return ResultNcmPlaceHolderNotFound;
                }
            } R_END_TRY_CATCH;
        }

        return ResultSuccess;
    }

    Result PlaceHolderAccessor::Open(FILE** out_handle, PlaceHolderId placeholder_id) {
        if (this->LoadFromCache(out_handle, placeholder_id)) {
            return ResultSuccess;
        }
        char placeholder_path[FS_MAX_PATH] = {0};

        this->GetPlaceHolderPath(placeholder_path, placeholder_id);

        FILE* f = nullptr;
        R_TRY(OpenFile(&f, placeholder_path, FS_OPEN_WRITE));

        *out_handle = f;
        return ResultSuccess;
    }

    Result PlaceHolderAccessor::SetSize(PlaceHolderId placeholder_id, size_t size) {
        char placeholder_path[FS_MAX_PATH] = {0};
        this->GetPlaceHolderPath(placeholder_path, placeholder_id);
        if (truncate(placeholder_path, size) == -1) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ResultFsPathNotFound) {
                    return ResultNcmPlaceHolderNotFound;
                }
            } R_END_TRY_CATCH;
        }

        return ResultSuccess;
    }

    Result PlaceHolderAccessor::GetSize(bool* found_in_cache, size_t* out_size, PlaceHolderId placeholder_id) {
        FILE* f = NULL;
        
        /* Set the scope for the scoped_lock. */
        {
            std::scoped_lock<HosMutex> lock(this->cache_mutex);
            
            if (placeholder_id == InvalidUuid) {
                *found_in_cache = false;
                return ResultSuccess;
            }

            CacheEntry* cache_entry = this->FindInCache(placeholder_id);

            if (cache_entry == nullptr) {
                *found_in_cache = false;
                return ResultSuccess;
            }

            cache_entry->id = InvalidUuid;
            f = cache_entry->handle;
        }

        this->StoreToCache(f, placeholder_id);
        
        if (fseek(f, 0L, SEEK_END) != 0) {
            return fsdevGetLastResult();
        }
        size_t size = ftell(f);
        if (fseek(f, 0L, SEEK_SET) != 0) {
            return fsdevGetLastResult();
        }

        *found_in_cache = true;
        *out_size = size;
        return ResultSuccess;
    }

    Result PlaceHolderAccessor::EnsureRecursively(PlaceHolderId placeholder_id) {
        char placeholder_path[FS_MAX_PATH] = {0};
        this->GetPlaceHolderPath(placeholder_path, placeholder_id);
        R_TRY(EnsureParentDirectoryRecursively(placeholder_path));
        return ResultSuccess;
    }

    bool PlaceHolderAccessor::LoadFromCache(FILE** out_handle, PlaceHolderId placeholder_id) {
        std::scoped_lock<HosMutex> lk(this->cache_mutex);
        CacheEntry *entry = this->FindInCache(placeholder_id);
        if (entry == nullptr) {
            return false;
        }
        entry->id = InvalidUuid;
        *out_handle = entry->handle;
        return true;
    }

    PlaceHolderAccessor::CacheEntry *PlaceHolderAccessor::FindInCache(PlaceHolderId placeholder_id) {
        if (placeholder_id == InvalidUuid) {
            return nullptr;
        }
        for (size_t i = 0; i < MaxCaches; i++) {
            if (placeholder_id == this->caches[i].id) {
                return &this->caches[i];
            }
        }
        return nullptr;
    }

    void PlaceHolderAccessor::StoreToCache(FILE* handle, PlaceHolderId placeholder_id) {
        std::scoped_lock<HosMutex> lk(this->cache_mutex);
        CacheEntry* cache = nullptr;

        /* Find an empty cache */
        for (size_t i = 0; i < MaxCaches; i++) {
            if (placeholder_id == InvalidUuid) {
                cache = &this->caches[i];
                break;
            }
        }

        /* No empty caches found. Let's clear cache 0. */
        if (cache == nullptr) {
            cache = &this->caches[0];

            /* Flush and close any handles. */
            if (cache->handle) {
                fflush(cache->handle);
                fclose(cache->handle);
            }
            cache->id = InvalidUuid;
        }

        /* Cache the new placeholder id and its file handle. */
        cache->id = placeholder_id;
        cache->handle = handle;
        cache->counter = this->cur_counter;
        this->cur_counter++;
    }

    void PlaceHolderAccessor::ClearAllCaches() {
        for (size_t i = 0; i < MaxCaches; i++) {
            CacheEntry* cache = &this->caches[i];

            if (cache->id != InvalidUuid) {
                fflush(cache->handle);
                fclose(cache->handle);
                cache->id = InvalidUuid;
            }
        }
    }

}