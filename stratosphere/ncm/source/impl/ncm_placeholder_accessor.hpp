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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "../ncm_types.hpp"
#include "../ncm_path_utils.hpp"

namespace sts::ncm::impl {

    class PlaceHolderAccessor {
        public:
            class CacheEntry {
                public:
                    PlaceHolderId id;
                    FILE* handle;
                    u64 counter;
            };

        public:
            static constexpr size_t MaxCaches = 0x2;

            CacheEntry caches[MaxCaches];
            char* root_path;
            u64 cur_counter;
            HosMutex cache_mutex;
            MakePlaceHolderPathFunc make_placeholder_path_func;
            bool delay_flush;

            PlaceHolderAccessor() : cur_counter(0), delay_flush(false) {
                for (size_t i = 0; i < MaxCaches; i++) {
                    caches[i].id = InvalidUuid;
                }
            }

            inline void GetPlaceHolderRootPath(char* out_placeholder_root) {
                path::GetPlaceHolderRootPath(out_placeholder_root, this->root_path);
            }

            inline void GetPlaceHolderPath(char* out_placeholder_path, PlaceHolderId placeholder_id) {
                char placeholder_root_path[FS_MAX_PATH] = {0};
                this->GetPlaceHolderRootPath(placeholder_root_path);
                this->make_placeholder_path_func(out_placeholder_path, placeholder_id, placeholder_root_path);
            }

            unsigned int GetDirectoryDepth();
            void GetPlaceHolderPathUncached(char* out_placeholder_path, PlaceHolderId placeholder_id);
            Result Create(PlaceHolderId placeholder_id, size_t size);
            Result Delete(PlaceHolderId placeholder_id);
            Result Open(FILE** out_handle, PlaceHolderId placeholder_id);
            Result SetSize(PlaceHolderId placeholder_id, size_t size);
            Result GetSize(bool* found_in_cache, size_t* out_size, PlaceHolderId placeholder_id);
            Result EnsureRecursively(PlaceHolderId placeholder_id);

            CacheEntry *FindInCache(PlaceHolderId placeholder_id);
            bool LoadFromCache(FILE** out_handle, PlaceHolderId placeholder_id);
            void FlushCache(FILE* handle, PlaceHolderId placeholder_id);
            void ClearAllCaches();
    };

}