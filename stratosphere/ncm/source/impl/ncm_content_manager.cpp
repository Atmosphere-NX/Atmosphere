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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/kvdb/kvdb_memory_key_value_store.hpp>
#include <optional>

#include "../ncm_contentmetadatabase.hpp"
#include "../ncm_contentstorage.hpp"
#include "../ncm_fs.hpp"
#include "../ncm_make_path.hpp"
#include "../ncm_readonlycontentstorage.hpp"
#include "ncm_content_manager.hpp"

namespace sts::ncm::impl {

    namespace {

        struct ContentStorageEntry {
            NON_COPYABLE(ContentStorageEntry);
            NON_MOVEABLE(ContentStorageEntry);

            char mount_point[16];
            char root_path[128];
            StorageId storage_id;
            FsContentStorageId content_storage_id;
            std::shared_ptr<IContentStorage> content_storage;

            inline ContentStorageEntry() : storage_id(StorageId::None),
                content_storage_id(FS_CONTENTSTORAGEID_NandSystem), content_storage(nullptr) {
                mount_point[0] = '\0';
                root_path[0] = '\0';
            }

            inline void Initialize(StorageId storage_id, FsContentStorageId content_storage_id) {
                this->storage_id = storage_id;
                this->content_storage_id = content_storage_id;
                this->content_storage = nullptr;
                MountName mount_name = CreateUniqueMountName();
                strcpy(this->mount_point, mount_name.name);
                snprintf(this->root_path, 0x80, "%s:/", this->mount_point);
            }
        };

        struct SaveDataMeta {
            u64 id;
            u64 size;
            u64 journal_size;
            u32 flags;
            FsSaveDataSpaceId space_id;
        } PACKED;

        static_assert(sizeof(SaveDataMeta) == 0x20, "SaveDataMeta definition!");

        struct ContentMetaDBEntry {
            NON_COPYABLE(ContentMetaDBEntry);
            NON_MOVEABLE(ContentMetaDBEntry);

            char mount_point[16];
            char meta_path[128];
            StorageId storage_id;
            SaveDataMeta save_meta;
            std::shared_ptr<IContentMetaDatabase> content_meta_database;
            std::optional<kvdb::MemoryKeyValueStore<ContentMetaKey>> kvs;
            u32 max_content_metas;

            inline ContentMetaDBEntry() : storage_id(StorageId::None), save_meta({0}), 
                content_meta_database(nullptr), kvs(std::nullopt), max_content_metas(0) {
                mount_point[0] = '\0';
                meta_path[0] = '\0';
            }

            Result Initialize(StorageId storage_id, SaveDataMeta& save_meta, size_t max_content_metas) {
                this->storage_id = storage_id;
                this->max_content_metas = max_content_metas;
                this->save_meta = save_meta;
                this->content_meta_database = nullptr;
                this->kvs.reset();
                MountName mount_name = CreateUniqueMountName();
                strcpy(this->mount_point, mount_name.name);
                this->mount_point[0] = '#';
                snprintf(this->meta_path, 0x80, "%s:/meta", this->mount_point);
                return ResultSuccess;
            }

            Result InitializeGameCard(size_t max_content_metas) {
                this->storage_id = StorageId::GameCard;
                this->max_content_metas = max_content_metas;
                this->content_meta_database = nullptr;
                this->kvs.reset();
                return ResultSuccess;
            }
        };

        constexpr size_t MaxContentStorageEntries = 8;
        constexpr size_t MaxContentMetaDBEntries = 8;

        HosMutex g_mutex;
        bool g_initialized = false;
        ContentStorageEntry g_content_storage_entries[MaxContentStorageEntries];
        ContentMetaDBEntry g_content_meta_entries[MaxContentMetaDBEntries];
        u32 g_num_content_storage_entries;
        u32 g_num_content_meta_entries;

        ContentStorageEntry* FindContentStorageEntry(StorageId storage_id) {
            for (size_t i = 0; i < MaxContentStorageEntries; i++) {
                ContentStorageEntry* entry = &g_content_storage_entries[i];

                if (entry->storage_id == storage_id) {
                    return entry;
                }
            }

            return nullptr;
        }

        ContentMetaDBEntry* FindContentMetaDBEntry(StorageId storage_id) {
            for (size_t i = 0; i < MaxContentMetaDBEntries; i++) {
                ContentMetaDBEntry* entry = &g_content_meta_entries[i];

                if (entry->storage_id == storage_id) {
                    return entry;
                }
            }

            return nullptr;
        }

    }

    Result InitializeContentManager() {
        std::scoped_lock<HosMutex> lk(g_mutex);

        /* Already initialized. */
        if (g_initialized) {
            return ResultSuccess;
        }
        
        size_t cur_storage_index = g_num_content_storage_entries;

        for (size_t i = 0; i < MaxContentStorageEntries; i++) {
            ContentStorageEntry* entry = &g_content_storage_entries[i];
            entry->storage_id = StorageId::None;
        }

        for (size_t i = 0; i < MaxContentMetaDBEntries; i++) {
            ContentMetaDBEntry* entry = &g_content_meta_entries[i];
            entry->storage_id = StorageId::None;
        }

        g_num_content_storage_entries++;
        auto storage_entry = &g_content_storage_entries[cur_storage_index];

        /* First, setup the NandSystem storage entry. */
        storage_entry->Initialize(StorageId::NandSystem, FS_CONTENTSTORAGEID_NandSystem);

        if (R_FAILED(VerifyContentStorage(StorageId::NandSystem))) {
            R_TRY(CreateContentStorage(StorageId::NandSystem));
        }

        R_TRY(ActivateContentStorage(StorageId::NandSystem));

        /* Next, the NandSystem content meta entry. */
        SaveDataMeta nand_system_save_meta;
        nand_system_save_meta.id = 0x8000000000000120;
        nand_system_save_meta.size = 0x6c000;
        nand_system_save_meta.journal_size = 0x6c000;
        nand_system_save_meta.flags = FsSaveDataFlags_SurviveFactoryReset| FsSaveDataFlags_SurviveFactoryResetForRefurbishment;
        nand_system_save_meta.space_id = FsSaveDataSpaceId_NandSystem;

        size_t cur_meta_index = g_num_content_meta_entries;
        g_num_content_meta_entries++;
        auto content_meta_entry = &g_content_meta_entries[cur_meta_index];

        R_TRY(content_meta_entry->Initialize(StorageId::NandSystem, nand_system_save_meta, 0x800));

        if (R_FAILED(VerifyContentMetaDatabase(StorageId::NandSystem))) {
            R_TRY(CreateContentMetaDatabase(StorageId::NandSystem));

            /* TODO: N supports a number of unused modes here, we don't bother implementing them currently. */
        }

        u32 current_flags = 0;
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_200 && R_SUCCEEDED(GetSaveDataFlags(&current_flags, 0x8000000000000120)) && current_flags != (FsSaveDataFlags_SurviveFactoryReset | FsSaveDataFlags_SurviveFactoryResetForRefurbishment)) {
            SetSaveDataFlags(0x8000000000000120, FsSaveDataSpaceId_NandSystem, FsSaveDataFlags_SurviveFactoryReset | FsSaveDataFlags_SurviveFactoryResetForRefurbishment);
        }
        
        R_TRY(ActivateContentMetaDatabase(StorageId::NandSystem));

        /* Now for NandUser's content storage entry. */
        cur_meta_index = g_num_content_meta_entries;
        g_num_content_meta_entries++;
        storage_entry = &g_content_storage_entries[cur_storage_index];
        storage_entry->Initialize(StorageId::NandUser, FS_CONTENTSTORAGEID_NandUser);

        /* And NandUser's content meta entry. */
        SaveDataMeta nand_user_save_meta;
        nand_user_save_meta.id = 0x8000000000000121;
        nand_user_save_meta.size = 0x29e000;
        nand_user_save_meta.journal_size = 0x29e000;
        nand_user_save_meta.flags = 0;
        nand_user_save_meta.space_id = FsSaveDataSpaceId_NandSystem;

        cur_meta_index = g_num_content_meta_entries;
        g_num_content_meta_entries++;
        content_meta_entry = &g_content_meta_entries[cur_meta_index];

        R_TRY(content_meta_entry->Initialize(StorageId::NandUser, nand_user_save_meta, 0x2000));

        /* 
           Beyond this point N no longer appears to bother
           incrementing the count for content storage entries or content meta entries. 
        */

        /* Next SdCard's content storage entry. */
        g_content_storage_entries[2].Initialize(StorageId::SdCard, FS_CONTENTSTORAGEID_SdCard);

        /* And SdCard's content meta entry. */
        SaveDataMeta sd_card_save_meta;
        sd_card_save_meta.id = 0x8000000000000124;
        sd_card_save_meta.size = 0xa08000;
        sd_card_save_meta.journal_size = 0xa08000;
        sd_card_save_meta.flags = 0;
        sd_card_save_meta.space_id = FsSaveDataSpaceId_SdCard;

        content_meta_entry = &g_content_meta_entries[2];
        R_TRY(content_meta_entry->Initialize(StorageId::SdCard, sd_card_save_meta, 0x2000));

        /* GameCard's content storage entry. */
        /* N doesn't set a content storage id for game cards, so we'll just use 0 (NandSystem). */
        g_content_storage_entries[3].Initialize(StorageId::GameCard, FS_CONTENTSTORAGEID_NandSystem);

        /* Lasty, GameCard's content meta entry. */
        content_meta_entry = &g_content_meta_entries[3];
        R_TRY(content_meta_entry->InitializeGameCard(0x800));

        g_initialized = true;
        return ResultSuccess;
    }

    void FinalizeContentManager() {
        {
            std::scoped_lock<HosMutex> lk(g_mutex);

            for (size_t i = 0; i < MaxContentStorageEntries; i++) {
                ContentStorageEntry* entry = &g_content_storage_entries[i];
                InactivateContentStorage(entry->storage_id);
            }

            for (size_t i = 0; i < MaxContentMetaDBEntries; i++) {
                ContentMetaDBEntry* entry = &g_content_meta_entries[i];
                InactivateContentMetaDatabase(entry->storage_id);
            }
        }

        for (size_t i = 0; i < MaxContentMetaDBEntries; i++) {
            ContentMetaDBEntry* entry = &g_content_meta_entries[i];
            entry->kvs.reset();
        }

        for (size_t i = 0; i < MaxContentStorageEntries; i++) {
            ContentStorageEntry* entry = &g_content_storage_entries[i];
            entry->content_storage = nullptr;
        }
    }

    Result CreateContentStorage(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentStorageEntry* entry = FindContentStorageEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        R_TRY(MountContentStorage(entry->mount_point, entry->content_storage_id));

        ON_SCOPE_EXIT {
            Unmount(entry->mount_point);
        };

        R_TRY(EnsureDirectoryRecursively(entry->root_path));
        R_TRY(EnsureContentAndPlaceHolderRoot(entry->root_path));

        return ResultSuccess;
    }

    Result VerifyContentStorage(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentStorageEntry* entry = FindContentStorageEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        MountName mount_name = CreateUniqueMountName();
        char mount_root[128] = {0};
        strcpy(mount_root, mount_name.name);
        strcat(mount_root, strchr(entry->root_path, ':'));
        R_TRY(MountContentStorage(mount_name.name, entry->content_storage_id));

        ON_SCOPE_EXIT {
            Unmount(mount_name.name);
        };

        R_TRY(CheckContentStorageDirectoriesExist(mount_root));

        return ResultSuccess;
    }

    Result OpenContentStorage(std::shared_ptr<IContentStorage>* out, StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentStorageEntry* entry = FindContentStorageEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }
        
        auto content_storage = entry->content_storage;

        if (!content_storage) {
            switch (storage_id) {
                case StorageId::GameCard:
                    return ResultNcmGameCardContentStorageNotActive;

                case StorageId::NandSystem:
                    return ResultNcmNandSystemContentStorageNotActive;

                case StorageId::NandUser:
                    return ResultNcmNandUserContentStorageNotActive;

                case StorageId::SdCard:
                    return ResultNcmSdCardContentStorageNotActive;

                default:
                    return ResultNcmUnknownContentStorageNotActive;
            }
        } 

        *out = std::move(content_storage);
        return ResultSuccess;
    }

    Result CloseContentStorageForcibly(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None) {
            return ResultNcmUnknownStorage;
        }
        
        ContentStorageEntry* entry = FindContentStorageEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        if (!entry->content_storage) {
            return ResultSuccess;
        }

        /* N doesn't bother checking the result of this */
        entry->content_storage->DisableForcibly();
        Unmount(entry->mount_point);
        entry->content_storage = nullptr;
        return ResultSuccess;
    }

    Result ActivateContentStorage(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentStorageEntry* entry = FindContentStorageEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        /* Already activated. */
        if (entry->content_storage != nullptr) {
            return ResultSuccess;
        }

        if (storage_id == StorageId::GameCard) {
            FsGameCardHandle gc_hnd;
            R_TRY(GetGameCardHandle(&gc_hnd));
            R_TRY(MountGameCardPartition(entry->mount_point, gc_hnd, FsGameCardPartiton_Secure));
            auto mount_guard = SCOPE_GUARD { Unmount(entry->mount_point); };
            auto content_storage = std::make_shared<ReadOnlyContentStorageInterface>();
            
            R_TRY(content_storage->Initialize(entry->root_path, path::MakeContentPathFlat));
            entry->content_storage = std::move(content_storage);
            mount_guard.Cancel();
        } else {
            R_TRY(MountContentStorage(entry->mount_point, entry->content_storage_id));
            auto mount_guard = SCOPE_GUARD { Unmount(entry->mount_point); };
            MakeContentPathFunc content_path_func = nullptr;
            MakePlaceHolderPathFunc placeholder_path_func = nullptr;
            bool delay_flush = false;
            auto content_storage = std::make_shared<ContentStorageInterface>();

            switch (storage_id) {
                case StorageId::NandSystem:
                    content_path_func = path::MakeContentPathFlat;
                    placeholder_path_func = path::MakePlaceHolderPathFlat;
                    break;

                case StorageId::SdCard:
                    delay_flush = true;
                default:
                    content_path_func = path::MakeContentPathHashByteLayered;
                    placeholder_path_func = path::MakePlaceHolderPathHashByteLayered;
                    break;
            }

            R_TRY(content_storage->Initialize(entry->root_path, content_path_func, placeholder_path_func, delay_flush));
            entry->content_storage = std::move(content_storage);
            mount_guard.Cancel();
        }

        return ResultSuccess;
    }

    Result InactivateContentStorage(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentStorageEntry* entry = FindContentStorageEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        /* Already inactivated. */
        if (entry->content_storage == nullptr) {
            return ResultSuccess;
        }

        entry->content_storage->DisableForcibly();
        entry->content_storage = nullptr;
        Unmount(entry->mount_point);
        return ResultSuccess;
    }

    Result CreateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || storage_id == StorageId::GameCard || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }

        ContentMetaDBEntry* entry = FindContentMetaDBEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        /* N doesn't bother checking the result of this. */
        fsDisableAutoSaveDataCreation();

        R_TRY_CATCH(MountSystemSaveData(entry->mount_point, entry->save_meta.space_id, entry->save_meta.id)) {
            R_CATCH(ResultFsTargetNotFound) {
                R_TRY(fsCreate_SystemSaveData(entry->save_meta.space_id, entry->save_meta.id, entry->save_meta.size, entry->save_meta.journal_size, entry->save_meta.flags));
                R_TRY(MountSystemSaveData(entry->mount_point, entry->save_meta.space_id, entry->save_meta.id));
            }
        } R_END_TRY_CATCH;

        ON_SCOPE_EXIT {
            Unmount(entry->mount_point);
        };

        R_TRY(EnsureDirectoryRecursively(entry->mount_point));
        R_TRY(fsdevCommitDevice(entry->mount_point));

        return ResultSuccess;
    }

    Result VerifyContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::GameCard) {
            return ResultSuccess;
        }

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }

        ContentMetaDBEntry* entry = FindContentMetaDBEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        bool mounted_save_data = false;

        if (!entry->content_meta_database) {
            R_TRY(MountSystemSaveData(entry->mount_point, entry->save_meta.space_id, entry->save_meta.id));
            mounted_save_data = true;
        }

        bool has_meta_path = false;
        R_TRY(HasDirectory(&has_meta_path, entry->meta_path));
        if (!has_meta_path) {
            return ResultNcmInvalidContentMetaDatabase;
        }

        if (mounted_save_data) {
            Unmount(entry->mount_point);
        }

        return ResultSuccess;
    }

    Result OpenContentMetaDatabase(std::shared_ptr<IContentMetaDatabase>* out, StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentMetaDBEntry* entry = FindContentMetaDBEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }
        
        std::shared_ptr<IContentMetaDatabase> content_meta_db = entry->content_meta_database;

        if (!content_meta_db) {
            switch (storage_id) {
                case StorageId::GameCard:
                    return ResultNcmGameCardContentMetaDatabaseNotActive;

                case StorageId::NandSystem:
                    return ResultNcmNandSystemContentMetaDatabaseNotActive;

                case StorageId::NandUser:
                    return ResultNcmNandUserContentMetaDatabaseNotActive;

                case StorageId::SdCard:
                    return ResultNcmSdCardContentMetaDatabaseNotActive;

                default:
                    return ResultNcmUnknownContentMetaDatabaseNotActive;
            }
        } 

        *out = std::move(content_meta_db);
        return ResultSuccess;
    }

    Result CloseContentMetaDatabaseForcibly(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None) {
            return ResultNcmUnknownStorage;
        }
        
        ContentMetaDBEntry* entry = FindContentMetaDBEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }
        
        std::shared_ptr<IContentMetaDatabase> content_meta_db = entry->content_meta_database;

        if (!content_meta_db) {
            return ResultSuccess;
        }

        /* N doesn't bother checking the result of this */
        content_meta_db->DisableForcibly();

        if (storage_id != StorageId::GameCard) {
            Unmount(entry->mount_point);
        }

        entry->content_meta_database = nullptr;
        entry->kvs.reset();
        return ResultSuccess;
    }

    Result CleanupContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentMetaDBEntry* entry = FindContentMetaDBEntry(storage_id);

        if (!entry) {
            return ResultNcmUnknownStorage;
        }

        R_TRY(fsDeleteSaveDataFileSystemBySaveDataSpaceId(entry->save_meta.space_id, entry->save_meta.id));
        return ResultSuccess;
    }

    Result ActivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentMetaDBEntry* entry = FindContentMetaDBEntry(storage_id);

        /* Already activated. */
        if (entry->content_meta_database != nullptr) {
            return ResultSuccess;
        }

        /* Make a brand new kvs. N doesn't quite do this, but we will for cleanliness. */
        entry->kvs.emplace();

        if (storage_id != StorageId::GameCard) {
            R_TRY(MountSystemSaveData(entry->mount_point, entry->save_meta.space_id, entry->save_meta.id));
            auto mount_guard = SCOPE_GUARD { Unmount(entry->mount_point); };
            R_TRY(entry->kvs->Initialize(entry->meta_path, entry->max_content_metas));
            R_TRY(entry->kvs->Load());

            auto content_meta_database = std::make_shared<ContentMetaDatabaseInterface>(&*entry->kvs, entry->mount_point);
            entry->content_meta_database = std::move(content_meta_database);
            mount_guard.Cancel();
        } else {
            R_TRY(entry->kvs->Initialize(entry->max_content_metas));
            R_TRY(entry->kvs->Load());
            auto content_meta_database = std::make_shared<OnMemoryContentMetaDatabaseInterface>(&*entry->kvs);
            entry->content_meta_database = std::move(content_meta_database);
        }

        return ResultSuccess;
    }

    Result InactivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock<HosMutex> lk(g_mutex);

        if (storage_id == StorageId::None || static_cast<u8>(storage_id) == 6) {
            return ResultNcmUnknownStorage;
        }
        
        ContentMetaDBEntry* entry = FindContentMetaDBEntry(storage_id);

        /* Already inactivated. */
        if (entry->content_meta_database == nullptr) {
            return ResultSuccess;
        }

        entry->content_meta_database->DisableForcibly();
        entry->content_meta_database = nullptr;
        /* This should lead to Index's destructor performing cleanup for us. */
        entry->kvs.reset();

        if (storage_id != StorageId::GameCard) {
            Unmount(entry->mount_point);
        }

        return ResultSuccess;
    }
}