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
#include "ncm_placeholder_accessor.hpp"
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace {

        constexpr inline const char * const BasePlaceHolderDirectory = "/placehld";

        constexpr inline const char * const PlaceHolderExtension = ".nca";
        constexpr inline size_t PlaceHolderExtensionLength = 4;

        constexpr inline size_t PlaceHolderFileNameLengthWithoutExtension = 2 * sizeof(PlaceHolderId);
        constexpr inline size_t PlaceHolderFileNameLength = PlaceHolderFileNameLengthWithoutExtension + PlaceHolderExtensionLength;

        void MakeBasePlaceHolderDirectoryPath(PathString *out, const char *root_path) {
            out->SetFormat("%s%s", root_path, BasePlaceHolderDirectory);
        }

        void MakePlaceHolderFilePath(PathString *out, PlaceHolderId id, MakePlaceHolderPathFunction func, const char *root_path) {
            PathString path;
            MakeBasePlaceHolderDirectoryPath(std::addressof(path), root_path);
            func(out, id, path);
        }

        ALWAYS_INLINE Result ConvertNotFoundResult(Result r) {
            R_TRY_CATCH(r) {
                R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultPlaceHolderNotFound())
            } R_END_TRY_CATCH;
            return ResultSuccess();
        }

    }

    void PlaceHolderAccessor::MakePath(PathString *out_placeholder_path, PlaceHolderId placeholder_id) const {
        MakePlaceHolderFilePath(out_placeholder_path, placeholder_id, m_make_placeholder_path_func, *m_root_path);
    }

    void PlaceHolderAccessor::MakeBaseDirectoryPath(PathString *out, const char *root_path) {
        MakeBasePlaceHolderDirectoryPath(out, root_path);
    }

    Result PlaceHolderAccessor::EnsurePlaceHolderDirectory(PlaceHolderId placeholder_id) {
        PathString path;
        this->MakePath(std::addressof(path), placeholder_id);
        return fs::EnsureParentDirectoryRecursively(path);
    }

    Result PlaceHolderAccessor::GetPlaceHolderIdFromFileName(PlaceHolderId *out, const char *name) {
        /* Ensure placeholder name is valid. */
        R_UNLESS(strnlen(name, PlaceHolderFileNameLength) == PlaceHolderFileNameLength,                                            ncm::ResultInvalidPlaceHolderFile());
        R_UNLESS(strncmp(name + PlaceHolderFileNameLengthWithoutExtension, PlaceHolderExtension, PlaceHolderExtensionLength) == 0, ncm::ResultInvalidPlaceHolderFile());

        /* Convert each character pair to a byte until we reach the end. */
        PlaceHolderId placeholder_id = {};
        for (size_t i = 0; i < sizeof(placeholder_id); i++) {
            char tmp[3];
            strlcpy(tmp, name + i * 2, sizeof(tmp));

            char *err = nullptr;
            reinterpret_cast<u8 *>(std::addressof(placeholder_id))[i] = static_cast<u8>(std::strtoul(tmp, std::addressof(err), 16));
            R_UNLESS(*err == '\x00', ncm::ResultInvalidPlaceHolderFile());
        }

        *out = placeholder_id;
        return ResultSuccess();
    }

    Result PlaceHolderAccessor::Open(fs::FileHandle *out_handle, PlaceHolderId placeholder_id) {
        /* Try to load from the cache. */
        R_SUCCEED_IF(this->LoadFromCache(out_handle, placeholder_id));

        /* Make the path of the placeholder. */
        PathString placeholder_path;
        this->MakePath(std::addressof(placeholder_path), placeholder_id);

        /* Open the placeholder file. */
        return fs::OpenFile(out_handle, placeholder_path, fs::OpenMode_Write);
    }

    bool PlaceHolderAccessor::LoadFromCache(fs::FileHandle *out_handle, PlaceHolderId placeholder_id) {
        std::scoped_lock lk(m_cache_mutex);

        /* Attempt to find an entry in the cache. */
        CacheEntry *entry = this->FindInCache(placeholder_id);
        if (!entry) {
            return false;
        }

        /* No cached entry found. */
        entry->id = InvalidPlaceHolderId;
        *out_handle = entry->handle;
        return true;
    }

    void PlaceHolderAccessor::StoreToCache(PlaceHolderId placeholder_id, fs::FileHandle handle) {
        std::scoped_lock lk(m_cache_mutex);

        /* Store placeholder id and file handle to a free entry. */
        CacheEntry *entry = this->GetFreeEntry();
        entry->id      = placeholder_id;
        entry->handle  = handle;
        entry->counter = m_cur_counter++;
    }

    void PlaceHolderAccessor::Invalidate(CacheEntry *entry) {
        /* Flush and close the cached entry's file. */
        if (entry != nullptr) {
            fs::FlushFile(entry->handle);
            fs::CloseFile(entry->handle);
            entry->id = InvalidPlaceHolderId;
        }
    }

    PlaceHolderAccessor::CacheEntry *PlaceHolderAccessor::FindInCache(PlaceHolderId placeholder_id) {
        /* Ensure placeholder id is valid. */
        if (placeholder_id == InvalidPlaceHolderId) {
            return nullptr;
        }

        /* Attempt to find a cache entry with the same placeholder id. */
        for (size_t i = 0; i < MaxCacheEntries; i++) {
            if (placeholder_id == m_caches[i].id) {
                return std::addressof(m_caches[i]);
            }
        }
        return nullptr;
    }

    PlaceHolderAccessor::CacheEntry *PlaceHolderAccessor::GetFreeEntry() {
        /* Try to find an already free entry. */
        for (size_t i = 0; i < MaxCacheEntries; i++) {
            if (m_caches[i].id == InvalidPlaceHolderId) {
                return std::addressof(m_caches[i]);
            }
        }

        /* Get the oldest entry. */
        CacheEntry *entry = std::addressof(m_caches[0]);
        for (size_t i = 1; i < MaxCacheEntries; i++) {
            if (entry->counter < m_caches[i].counter) {
                entry = std::addressof(m_caches[i]);
            }
        }
        this->Invalidate(entry);
        return entry;
    }

    void PlaceHolderAccessor::GetPath(PathString *placeholder_path, PlaceHolderId placeholder_id) {
        {
            std::scoped_lock lock(m_cache_mutex);
            this->Invalidate(this->FindInCache(placeholder_id));
        }
        this->MakePath(placeholder_path, placeholder_id);
    }

    Result PlaceHolderAccessor::CreatePlaceHolderFile(PlaceHolderId placeholder_id, s64 size) {
        /* Ensure the destination directory exists. */
        R_TRY(this->EnsurePlaceHolderDirectory(placeholder_id));

        /* Get the placeholder path. */
        PathString placeholder_path;
        this->GetPath(std::addressof(placeholder_path), placeholder_id);

        /* Create the placeholder file. */
        R_TRY_CATCH(fs::CreateFile(placeholder_path, size, fs::CreateOption_BigFile)) {
            R_CONVERT(fs::ResultPathAlreadyExists, ncm::ResultPlaceHolderAlreadyExists())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result PlaceHolderAccessor::DeletePlaceHolderFile(PlaceHolderId placeholder_id) {
        /* Get the placeholder path. */
        PathString placeholder_path;
        this->GetPath(std::addressof(placeholder_path), placeholder_id);

        /* Delete the placeholder file. */
        R_TRY_CATCH(fs::DeleteFile(placeholder_path)) {
            R_CONVERT(fs::ResultPathNotFound, ncm::ResultPlaceHolderNotFound())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result PlaceHolderAccessor::WritePlaceHolderFile(PlaceHolderId placeholder_id, s64 offset, const void *buffer, size_t size) {
        /* Open the placeholder file. */
        fs::FileHandle file;
        R_TRY_CATCH(this->Open(std::addressof(file), placeholder_id)) {
            R_CONVERT(fs::ResultPathNotFound, ncm::ResultPlaceHolderNotFound())
        } R_END_TRY_CATCH;

        /* Store opened files to the cache regardless of write failures. */
        ON_SCOPE_EXIT { this->StoreToCache(placeholder_id, file); };

        /* Write data to the placeholder file. */
        return fs::WriteFile(file, offset, buffer, size, m_delay_flush ? fs::WriteOption::Flush : fs::WriteOption::None);
    }

    Result PlaceHolderAccessor::SetPlaceHolderFileSize(PlaceHolderId placeholder_id, s64 size) {
        /* Open the placeholder file. */
        fs::FileHandle file;
        R_TRY_CATCH(this->Open(std::addressof(file), placeholder_id)) {
            R_CONVERT(fs::ResultPathNotFound, ncm::ResultPlaceHolderNotFound())
        } R_END_TRY_CATCH;

        /* Close the file on exit. */
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Set the size of the placeholder file. */
        return fs::SetFileSize(file, size);
    }

    Result PlaceHolderAccessor::TryGetPlaceHolderFileSize(bool *found_in_cache, s64 *out_size, PlaceHolderId placeholder_id) {
        /* Attempt to find the placeholder in the cache. */
        fs::FileHandle handle;
        auto found = this->LoadFromCache(std::addressof(handle), placeholder_id);

        if (found) {
            /* Renew the entry in the cache. */
            this->StoreToCache(placeholder_id, handle);
            R_TRY(fs::GetFileSize(out_size, handle));
            *found_in_cache = true;
        } else {
            *found_in_cache = false;
        }

        return ResultSuccess();
    }

    void PlaceHolderAccessor::InvalidateAll() {
        /* Invalidate all cache entries. */
        for (auto &entry : m_caches) {
            if (entry.id != InvalidPlaceHolderId) {
                this->Invalidate(std::addressof(entry));
            }
        }
    }

}
