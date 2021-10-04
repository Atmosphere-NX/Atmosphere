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

namespace ams::ncm {

    Result SelectDownloadableStorage(StorageId *out_storage_id, StorageId storage_id, s64 required_size) {
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

    Result SelectPatchStorage(StorageId *out_storage_id, StorageId storage_id, PatchId patch_id) {
        auto list = GetStorageList(storage_id);
        u32 version = 0;
        *out_storage_id = storage_id;

        for (s32 i = 0; i < list.Count(); i++) {
            auto candidate = list[i];

            /* Open the content meta database. */
            ContentMetaDatabase content_meta_database;
            if (R_FAILED(ncm::OpenContentMetaDatabase(std::addressof(content_meta_database), candidate))) {
                continue;
            }

            /* Get the latest key. */
            ContentMetaKey key;
            R_TRY_CATCH(content_meta_database.GetLatest(std::addressof(key), patch_id.value)) {
                R_CATCH(ncm::ResultContentMetaNotFound) { continue; }
            } R_END_TRY_CATCH;

            if (key.version > version) {
                version = key.version;
                *out_storage_id = candidate;
            }
        }

        return ResultSuccess();
    }

    namespace {

        constexpr const char * const StorageIdStrings[] = {
            "None",
            "Host",
            "GameCard",
            "BuiltInSystem",
            "BuiltInUser",
            "SdCard"
        };

        constexpr const char * const StorageIdStringsForPlayReport[] = {
            "None",
            "Host",
            "Card",
            "BuildInSystem",
            "BuildInUser",
            "SdCard"
        };

    }

    const char *GetStorageIdString(StorageId storage_id) {
        switch (storage_id) {
            case StorageId::None:          return "None";
            case StorageId::Host:          return "Host";
            case StorageId::GameCard:      return "GameCard";
            case StorageId::BuiltInSystem: return "BuiltInSystem";
            case StorageId::BuiltInUser:   return "BuiltInUser";
            default:                       return "(unknown)";
        }
    }

    const char *GetStorageIdStringForPlayReport(StorageId storage_id) {
        switch (storage_id) {
            case StorageId::None:          return "None";
            case StorageId::Host:          return "Host";
            case StorageId::Card:          return "Card";
            case StorageId::BuildInSystem: return "BuildInSystem";
            case StorageId::BuildInUser:   return "BuildInUser";
            default:                       return "(unknown)";
        }
    }

}
