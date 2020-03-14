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
#include <stratosphere.hpp>

namespace ams::ncm {

    namespace {

        constexpr const char *StorageIdStrings[] = {
            "None",
            "Host",
            "GameCard",
            "BuiltInSystem",
            "BuiltInUser",
            "SdCard"
        };

        constexpr const char *StorageIdStringsForPlayReport[] = {
            "None",
            "Host",
            "Card",
            "BuildInSystem",
            "BuildInUser",
            "SdCard"
        };

    }

    Result SelectDownloadableStorage(StorageId *out_storage_id, StorageId storage_id, size_t required_size) {
        auto list = GetStorageList(storage_id);
        for (s32 i = 0; i < list.Count(); i++) {
            auto candidate = list[i];

            /* Open the content meta database. NOTE: This is unused. */
            ContentMetaDatabase content_meta_database;
            if (R_FAILED(ncm::OpenContentMetaDatabase(std::addressof(content_meta_database), candidate))) {
                continue;
            }

            /* Open the content storage. */
            ContentStorage content_storage;
            if (R_FAILED(ncm::OpenContentStorage(std::addressof(content_storage), candidate))) {
                continue;
            }

            /* Get the free space on this storage. */
            s64 free_space_size;
            R_TRY(content_storage.GetFreeSpaceSize(std::addressof(free_space_size)));

            /* There must be more free space than is required. */
            if (free_space_size <= required_size) {
                continue;
            }

            /* Output the storage id. */
            *out_storage_id = storage_id;
            return ResultSuccess();
        }

        return ncm::ResultNotEnoughInstallSpace();
    }

    const char *GetStorageIdString(StorageId storage_id) {
        u8 id = static_cast<u8>(storage_id);
        return id > 5 ? "(unknown)" : StorageIdStrings[id];
    }
    
    const char *GetStorageIdStringForPlayReport(StorageId storage_id) {
        u8 id = static_cast<u8>(storage_id);
        return id > 5 ? "(unknown)" : StorageIdStringsForPlayReport[id];
    }

}
