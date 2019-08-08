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

namespace sts::ncm::impl {

    Result PlaceHolderAccessor::Open(FILE** out_handle, PlaceHolderId placeholder_id) {
        if (this->LoadFromCache(out_handle, placeholder_id)) {
            return ResultSuccess;
        }
        char placeholder_path[FS_MAX_PATH] = {0};
        this->MakePath(placeholder_path, placeholder_id);

        FILE* f = nullptr;
        R_TRY(OpenFile(&f, placeholder_path, FS_OPEN_WRITE));

        *out_handle = f;
        return ResultSuccess;
    }

    bool PlaceHolderAccessor::LoadFromCache(FILE** out_handle, PlaceHolderId placeholder_id) {
        std::scoped_lock lk(this->cache_mutex);
        CacheEntry *entry = this->FindInCache(placeholder_id);
        if (!entry) {
            return false;
        }
        *out_handle = entry->handle;
        entry->id = InvalidUuid;
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
        CacheEntry* entry = this->FindInCache(InvalidUuid);
        
        if (entry) {
            return entry;
        }

        /* Get the oldest entry. */
        {
            entry = &this->caches[0];
            for (size_t i = 1; i < MaxCaches; i++) {
                if (entry->counter > this->caches[i].counter) {
                    entry = &this->caches[i];
                }
            }
            this->Invalidate(entry);
            return entry;
        }
    }

    void PlaceHolderAccessor::StoreToCache(FILE* handle, PlaceHolderId placeholder_id) {
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
            entry->id = InvalidUuid;
        }
    }

    void PlaceHolderAccessor::Initialize(char* root, MakePlaceHolderPathFunc path_func, bool delay_flush) {
        this->root_path = root;
        this->make_placeholder_path_func = path_func;
        this->delay_flush = delay_flush;
    }

    unsigned int PlaceHolderAccessor::GetDirectoryDepth() {
        if (this->make_placeholder_path_func == static_cast<MakePlaceHolderPathFunc>(path::MakePlaceHolderPathFlat)) {
            return 1;
        } else if (this->make_placeholder_path_func == static_cast<MakePlaceHolderPathFunc>(path::MakePlaceHolderPathHashByteLayered)) {
            return 2;
        }

        std::abort();
    }

    void PlaceHolderAccessor::GetPath(char* placeholder_path_out, PlaceHolderId placeholder_id) {
        std::scoped_lock lock(this->cache_mutex);
        CacheEntry* entry = this->FindInCache(placeholder_id);
        this->Invalidate(entry);
        this->MakePath(placeholder_path_out, placeholder_id);
    }

    Result PlaceHolderAccessor::Create(PlaceHolderId placeholder_id, size_t size) {
        char placeholder_path[FS_MAX_PATH] = {0};

        this->EnsureRecursively(placeholder_id);
        this->GetPath(placeholder_path, placeholder_id);

        R_TRY_CATCH(fsdevCreateFile(placeholder_path, size, FS_CREATE_BIG_FILE)) {
            R_CATCH(ResultFsPathAlreadyExists) {
                return ResultNcmPlaceHolderAlreadyExists;
            }
        } R_END_TRY_CATCH;

        return ResultSuccess;
    }

    Result PlaceHolderAccessor::Delete(PlaceHolderId placeholder_id) {
        char placeholder_path[FS_MAX_PATH] = {0};

        this->GetPath(placeholder_path, placeholder_id);

        if (std::remove(placeholder_path) != 0) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ResultFsPathNotFound) {
                    return ResultNcmPlaceHolderNotFound;
                }
            } R_END_TRY_CATCH;
        }

        return ResultSuccess;
    }

    Result PlaceHolderAccessor::Write(PlaceHolderId placeholder_id, size_t offset, const void* buffer, size_t size) {
        FILE* f = nullptr;

        R_TRY_CATCH(this->Open(&f, placeholder_id)) {
            R_CATCH(ResultFsPathNotFound) {
                return ResultNcmPlaceHolderNotFound;
            }
        } R_END_TRY_CATCH;

        ON_SCOPE_EXIT {
            this->StoreToCache(f, placeholder_id);
        };

        R_TRY(WriteFile(f, offset, buffer, size, !this->delay_flush));
        return ResultSuccess;
    }

    Result PlaceHolderAccessor::SetSize(PlaceHolderId placeholder_id, size_t size) {
        char placeholder_path[FS_MAX_PATH] = {0};
        this->MakePath(placeholder_path, placeholder_id);
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
            std::scoped_lock lock(this->cache_mutex);
            
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
        this->MakePath(placeholder_path, placeholder_id);
        R_TRY(EnsureParentDirectoryRecursively(placeholder_path));
        return ResultSuccess;
    }

    void PlaceHolderAccessor::InvalidateAll() {
        for (auto &entry : this->caches) {
            if (entry.id != InvalidUuid) {
                this->Invalidate(&entry);
            }
        }
    }

}