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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "../ncm_path_utils.hpp"

namespace ams::ncm::impl {

    class PlaceHolderAccessor {
        private:
            class CacheEntry {
                public:
                    PlaceHolderId id;
                    FILE *handle;
                    u64 counter;
            };
        private:
            static constexpr size_t MaxCaches = 0x2;

            std::array<CacheEntry, MaxCaches> caches;
            PathString root_path;
            u64 cur_counter;
            os::Mutex cache_mutex;
            MakePlaceHolderPathFunc make_placeholder_path_func;
            bool delay_flush;
        private:
            Result Open(FILE** out_handle, PlaceHolderId placeholder_id);
            CacheEntry *FindInCache(PlaceHolderId placeholder_id);
            bool LoadFromCache(FILE** out_handle, PlaceHolderId placeholder_id);
            CacheEntry *GetFreeEntry();
            void StoreToCache(FILE *handle, PlaceHolderId placeholder_id);
            void Invalidate(CacheEntry *entry);
        public:
            PlaceHolderAccessor() : cur_counter(0), delay_flush(false) {
                for (size_t i = 0; i < MaxCaches; i++) {
                    caches[i].id = InvalidPlaceHolderId;
                }
            }

            inline void MakeRootPath(PathString *placeholder_root) {
                path::GetPlaceHolderRootPath(placeholder_root, this->root_path);
            }

            inline void MakePath(PathString *placeholder_path, PlaceHolderId placeholder_id) {
                PathString root_path;
                this->MakeRootPath(std::addressof(root_path));
                this->make_placeholder_path_func(placeholder_path, placeholder_id, root_path);
            }

            void Initialize(const char *root, MakePlaceHolderPathFunc path_func, bool delay_flush);
            unsigned int GetDirectoryDepth();
            void GetPath(PathString *out_placeholder_path, PlaceHolderId placeholder_id);
            Result Create(PlaceHolderId placeholder_id, size_t size);
            Result Delete(PlaceHolderId placeholder_id);
            Result Write(PlaceHolderId placeholder_id, size_t offset, const void *buffer, size_t size);
            Result SetSize(PlaceHolderId placeholder_id, size_t size);
            Result GetSize(bool *found_in_cache, size_t *out_size, PlaceHolderId placeholder_id);
            Result EnsureRecursively(PlaceHolderId placeholder_id);
            void InvalidateAll();
    };

}
