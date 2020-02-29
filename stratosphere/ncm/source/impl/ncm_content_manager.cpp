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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/kvdb/kvdb_memory_key_value_store.hpp>
#include <optional>

#include "../ncm_content_meta_database.hpp"
#include "../ncm_content_storage.hpp"
#include "../ncm_fs.hpp"
#include "../ncm_make_path.hpp"
#include "../ncm_read_only_content_storage.hpp"
#include "ncm_content_manager.hpp"
#include "ncm_rights_cache.hpp"

namespace ams::ncm::impl {

    namespace {

        struct ContentStorageRoot {
            NON_COPYABLE(ContentStorageRoot);
            NON_MOVEABLE(ContentStorageRoot);

            char mount_point[16];
            char path[128];
            StorageId storage_id;
            FsContentStorageId content_storage_id;
            std::shared_ptr<IContentStorage> content_storage;

            inline ContentStorageRoot() : storage_id(StorageId::None),
                content_storage_id(FsContentStorageId_System), content_storage(nullptr) {
                mount_point[0] = '\0';
                path[0] = '\0';
            }

            inline void Initialize(StorageId storage_id, FsContentStorageId content_storage_id) {
                this->storage_id = storage_id;
                this->content_storage_id = content_storage_id;
                this->content_storage = nullptr;
                MountName mount_name = fs::CreateUniqueMountName();
                std::strcpy(this->mount_point, mount_name.name);
                snprintf(this->path, 0x80, "%s:/", this->mount_point);
            }
        };

        struct SaveDataMeta {
            u64 id;
            u64 size;
            u64 journal_size;
            u32 flags;
            FsSaveDataSpaceId space_id;
        };

        static_assert(sizeof(SaveDataMeta) == 0x20, "SaveDataMeta definition!");

        constexpr u64 BuiltInSystemSaveDataId             = 0x8000000000000120;
        constexpr u64 BuiltInSystemSaveDataSize           = 0x6c000;
        constexpr u64 BuiltInSystemSaveDataJournalSize    = 0x6c000;
        constexpr u32 BuiltInSystemSaveDataFlags          = FsSaveDataFlags_KeepAfterResettingSystemSaveData | FsSaveDataFlags_KeepAfterRefurbishment;
        
        constexpr SaveDataMeta BuiltInSystemSaveDataMeta = {
            .id              = BuiltInSystemSaveDataId,
            .size            = BuiltInSystemSaveDataSize,
            .journal_size    = BuiltInSystemSaveDataJournalSize,
            .flags           = BuiltInSystemSaveDataFlags,
            .space_id        = FsSaveDataSpaceId_System
        };

        constexpr u64 BuiltInUserSaveDataId             = 0x8000000000000121;
        constexpr u64 BuiltInUserSaveDataSize           = 0x29e000;
        constexpr u64 BuiltInUserSaveDataJournalSize    = 0x29e000;
        constexpr u32 BuiltInUserSaveDataFlags          = 0;

        constexpr SaveDataMeta BuiltInUserSaveDataMeta = {
            .id              = BuiltInUserSaveDataId,
            .size            = BuiltInUserSaveDataSize,
            .journal_size    = BuiltInUserSaveDataJournalSize,
            .flags           = BuiltInUserSaveDataFlags,
            .space_id        = FsSaveDataSpaceId_System
        };

        constexpr u64 SdCardSaveDataId             = 0x8000000000000124;
        constexpr u64 SdCardSaveDataSize           = 0xa08000;
        constexpr u64 SdCardSaveDataJournalSize    = 0xa08000;
        constexpr u32 SdCardSaveDataFlags          = 0;

        constexpr SaveDataMeta SdCardSaveDataMeta = {
            .id              = SdCardSaveDataId,
            .size            = SdCardSaveDataSize,
            .journal_size    = SdCardSaveDataJournalSize,
            .flags           = SdCardSaveDataFlags,
            .space_id        = FsSaveDataSpaceId_SdSystem,
        };

        struct ContentMetaDatabaseEntry {
            NON_COPYABLE(ContentMetaDatabaseEntry);
            NON_MOVEABLE(ContentMetaDatabaseEntry);

            char mount_point[16];
            char meta_path[128];
            StorageId storage_id;
            SaveDataMeta save_meta;
            std::shared_ptr<IContentMetaDatabase> content_meta_database;
            std::optional<kvdb::MemoryKeyValueStore<ContentMetaKey>> kvs;
            u32 max_content_metas;

            inline ContentMetaDatabaseEntry() : storage_id(StorageId::None), save_meta({0}), 
                content_meta_database(nullptr), kvs(std::nullopt), max_content_metas(0) {
                mount_point[0] = '\0';
                meta_path[0] = '\0';
            }

            Result Initialize(StorageId storage_id, const SaveDataMeta& save_meta, size_t max_content_metas) {
                this->storage_id = storage_id;
                this->max_content_metas = max_content_metas;
                this->save_meta = save_meta;
                this->content_meta_database = nullptr;
                this->kvs = std::nullopt;
                MountName mount_name = fs::CreateUniqueMountName();
                strcpy(this->mount_point, mount_name.name);
                this->mount_point[0] = '#';
                snprintf(this->meta_path, 0x80, "%s:/meta", this->mount_point);
                return ResultSuccess();
            }

            Result InitializeGameCard(size_t max_content_metas) {
                this->storage_id = StorageId::GameCard;
                this->max_content_metas = max_content_metas;
                this->content_meta_database = nullptr;
                this->kvs = std::nullopt;
                return ResultSuccess();
            }
        };

        constexpr size_t MaxBuiltInSystemContentMetaCount = 0x800;
        constexpr size_t MaxBuiltInUserContentMetaCount   = 0x2000;
        constexpr size_t MaxSdCardContentMetaCount        = 0x2000;

        constexpr size_t MaxContentStorageEntries      = 8;
        constexpr size_t MaxContentMetaDatabaseEntries = 8;

        os::Mutex g_mutex;
        bool g_initialized = false;
        ContentStorageRoot g_content_storage_roots[MaxContentStorageEntries];
        ContentMetaDatabaseEntry g_content_meta_entries[MaxContentMetaDatabaseEntries];
        u32 g_num_content_storage_entries;
        u32 g_num_content_meta_entries;
        RightsIdCache g_rights_id_cache;

        constexpr inline bool IsUniqueStorage(StorageId id) {
            return id != StorageId::None && id != StorageId::Any;
        }

        Result FindContentStorageRoot(ContentStorageRoot **out, StorageId storage_id) {
            for (size_t i = 0; i < MaxContentStorageEntries; i++) {
                ContentStorageRoot *root = &g_content_storage_roots[i];

                if (root->storage_id == storage_id) {
                    *out = root;
                    return ResultSuccess();
                }
            }

            return ncm::ResultUnknownStorage();
        }

        Result GetUniqueContentStorageRoot(ContentStorageRoot **out, StorageId storage_id) {
            R_UNLESS(IsUniqueStorage(storage_id), ncm::ResultUnknownStorage());
            return FindContentStorageRoot(out, storage_id);
        }

        Result FindContentMetaDatabaseEntry(ContentMetaDatabaseEntry **out, StorageId storage_id) {
            for (size_t i = 0; i < MaxContentMetaDatabaseEntries; i++) {
                ContentMetaDatabaseEntry *entry = &g_content_meta_entries[i];

                if (entry->storage_id == storage_id) {
                    *out = entry;
                    return ResultSuccess();
                }
            }

            return ncm::ResultUnknownStorage();
        }

        Result GetUniqueContentMetaDatabaseEntry(ContentMetaDatabaseEntry **out, StorageId storage_id) {
            R_UNLESS(IsUniqueStorage(storage_id), ncm::ResultUnknownStorage());
            return FindContentMetaDatabaseEntry(out, storage_id);
        }

        ALWAYS_INLINE Result GetContentStorageNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:
                    return ResultGameCardContentStorageNotActive();
                case StorageId::BuiltInSystem:
                    return ResultNandSystemContentStorageNotActive();
                case StorageId::BuiltInUser:
                    return ResultNandUserContentStorageNotActive();
                case StorageId::SdCard:
                    return ResultSdCardContentStorageNotActive();
                default:
                    return ResultUnknownContentStorageNotActive();
            }
        }

        ALWAYS_INLINE Result GetContentMetaDatabaseNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:
                    return ResultGameCardContentMetaDatabaseNotActive();
                case StorageId::BuiltInSystem:
                    return ResultNandSystemContentMetaDatabaseNotActive();
                case StorageId::BuiltInUser:
                    return ResultNandUserContentMetaDatabaseNotActive();
                case StorageId::SdCard:
                    return ResultSdCardContentMetaDatabaseNotActive();
                default:
                    return ResultUnknownContentMetaDatabaseNotActive();
            }
        }

        Result EnsureAndMountSystemSaveData(const char *mount_name, const SaveDataMeta &save_meta) {
            R_TRY_CATCH(fs::MountSystemSaveData(mount_name, save_meta.space_id, save_meta.id)) {
                R_CATCH(ams::fs::ResultTargetNotFound) {
                    R_TRY(fsCreate_SystemSaveData(save_meta.space_id, save_meta.id, save_meta.size, save_meta.journal_size, save_meta.flags));
                    R_TRY(fs::MountSystemSaveData(mount_name, save_meta.space_id, save_meta.id));
                }
            } R_END_TRY_CATCH;
            return ResultSuccess();
        }

        inline void ReplaceMountName(char *out_path, const char *mount_name, const char *root_path) {
            strcpy(out_path, mount_name);
            strcat(out_path, strchr(root_path, ':'));
        }

    }

    Result InitializeContentManager() {
        std::scoped_lock lk(g_mutex);

        /* Already initialized. */
        R_UNLESS(!g_initialized, ResultSuccess());

        for (size_t i = 0; i < MaxContentStorageEntries; i++) {
            ContentStorageRoot *entry = &g_content_storage_roots[i];
            entry->storage_id = StorageId::None;
        }

        for (size_t i = 0; i < MaxContentMetaDatabaseEntries; i++) {
            ContentMetaDatabaseEntry *entry = &g_content_meta_entries[i];
            entry->storage_id = StorageId::None;
        }

        /* First, setup the BuiltInSystem storage entry. */
        g_content_storage_roots[g_num_content_storage_entries++].Initialize(StorageId::BuiltInSystem, FsContentStorageId_System);

        if (R_FAILED(VerifyContentStorage(StorageId::BuiltInSystem))) {
            R_TRY(CreateContentStorage(StorageId::BuiltInSystem));
        }

        R_TRY(ActivateContentStorage(StorageId::BuiltInSystem));

        /* Next, the BuiltInSystem content meta entry. */
        R_TRY(g_content_meta_entries[g_num_content_meta_entries++].Initialize(StorageId::BuiltInSystem, BuiltInSystemSaveDataMeta, MaxBuiltInSystemContentMetaCount));

        if (R_FAILED(VerifyContentMetaDatabase(StorageId::BuiltInSystem))) {
            R_TRY(CreateContentMetaDatabase(StorageId::BuiltInSystem));

            /* TODO: N supports a number of unused modes here, we don't bother implementing them currently. */
        }

        u32 current_flags = 0;
        if (hos::GetVersion() >= hos::Version_200 && R_SUCCEEDED(fs::GetSaveDataFlags(&current_flags, BuiltInSystemSaveDataId)) && current_flags != (FsSaveDataFlags_KeepAfterResettingSystemSaveData | FsSaveDataFlags_KeepAfterRefurbishment)) {
            fs::SetSaveDataFlags(BuiltInSystemSaveDataId, FsSaveDataSpaceId_System, FsSaveDataFlags_KeepAfterResettingSystemSaveData | FsSaveDataFlags_KeepAfterRefurbishment);
        }
        
        R_TRY(ActivateContentMetaDatabase(StorageId::BuiltInSystem));

        /* Now for BuiltInUser's content storage and content meta entries. */
        g_content_storage_roots[g_num_content_storage_entries++].Initialize(StorageId::BuiltInUser, FsContentStorageId_User);
        R_TRY(g_content_meta_entries[g_num_content_meta_entries++].Initialize(StorageId::BuiltInUser, BuiltInUserSaveDataMeta, MaxBuiltInUserContentMetaCount));

        /* Beyond this point N no longer appears to bother */
        /* incrementing the count for content storage entries or content meta entries. */

        /* Next SdCard's content storage and content meta entries. */
        g_content_storage_roots[2].Initialize(StorageId::SdCard, FsContentStorageId_SdCard);
        R_TRY(g_content_meta_entries[2].Initialize(StorageId::SdCard, SdCardSaveDataMeta, MaxSdCardContentMetaCount));

        /* GameCard's content storage and content meta entries. */
        /* N doesn't set a content storage id for game cards, so we'll just use 0 (System). */
        g_content_storage_roots[3].Initialize(StorageId::GameCard, FsContentStorageId_System);
        R_TRY(g_content_meta_entries[3].InitializeGameCard(0x800));

        g_initialized = true;
        return ResultSuccess();
    }

    void FinalizeContentManager() {
        {
            std::scoped_lock lk(g_mutex);

            for (size_t i = 0; i < MaxContentStorageEntries; i++) {
                ContentStorageRoot *entry = &g_content_storage_roots[i];
                InactivateContentStorage(entry->storage_id);
            }

            for (size_t i = 0; i < MaxContentMetaDatabaseEntries; i++) {
                ContentMetaDatabaseEntry *entry = &g_content_meta_entries[i];
                InactivateContentMetaDatabase(entry->storage_id);
            }
        }

        for (size_t i = 0; i < MaxContentMetaDatabaseEntries; i++) {
            ContentMetaDatabaseEntry *entry = &g_content_meta_entries[i];
            entry->kvs.reset();
        }

        for (size_t i = 0; i < MaxContentStorageEntries; i++) {
            ContentStorageRoot *entry = &g_content_storage_roots[i];
            entry->content_storage = nullptr;
        }
    }

    Result CreateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        R_TRY(fs::MountContentStorage(root->mount_point, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_point); };

        R_TRY(fs::EnsureDirectoryRecursively(root->path));
        R_TRY(fs::EnsureContentAndPlaceHolderRoot(root->path));

        return ResultSuccess();
    }

    Result VerifyContentStorage(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        char mount_root[0x80] = {};
        auto mount_name = fs::CreateUniqueMountName();  /* should this be fs::? should it be ncm::? ncm::impl? */
        ReplaceMountName(mount_root, mount_name.name, root->path);

        R_TRY(fs::MountContentStorage(mount_name.name, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(mount_name.name); };

        R_TRY(fs::CheckContentStorageDirectoriesExist(mount_root));

        return ResultSuccess();
    }

    Result OpenContentStorage(std::shared_ptr<IContentStorage> *out, StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));
        
        auto content_storage = root->content_storage;

        if (hos::GetVersion() >= hos::Version_200) {
            R_UNLESS(content_storage, GetContentStorageNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content storages as soon as they are opened. */
            if (!content_storage) {
                R_TRY(ActivateContentStorage(storage_id));
                content_storage = root->content_storage;
            }
        }

        *out = std::move(content_storage);
        return ResultSuccess();
    }

    Result CloseContentStorageForcibly(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        R_UNLESS(storage_id != StorageId::None, ncm::ResultUnknownStorage());
        ContentStorageRoot *root;
        R_TRY(FindContentStorageRoot(std::addressof(root), storage_id));

        if (root->content_storage) {
            /* N doesn't bother checking the result of this */
            root->content_storage->DisableForcibly();
            fs::Unmount(root->mount_point);
            root->content_storage = nullptr;
        }

        return ResultSuccess();
    }

    Result ActivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        /* Already activated. */
        R_UNLESS(root->content_storage == nullptr, ResultSuccess());

        if (storage_id == StorageId::GameCard) {
            FsGameCardHandle gc_hnd;
            R_TRY(fs::GetGameCardHandle(&gc_hnd));
            R_TRY(fs::MountGameCardPartition(root->mount_point, gc_hnd, FsGameCardPartition_Secure));
        } else {
            R_TRY(fs::MountContentStorage(root->mount_point, root->content_storage_id));
        }

        auto mount_guard = SCOPE_GUARD { fs::Unmount(root->mount_point); };

        if (storage_id == StorageId::GameCard) {
            auto content_storage = std::make_shared<ReadOnlyContentStorageInterface>();
            R_TRY(content_storage->Initialize(root->path, path::MakeContentPathFlat));
            root->content_storage = std::move(content_storage);
        } else {
            MakeContentPathFunc content_path_func = nullptr;
            MakePlaceHolderPathFunc placeholder_path_func = nullptr;
            bool delay_flush = false;
            auto content_storage = std::make_shared<ContentStorageInterface>();

            switch (storage_id) {
                case StorageId::BuiltInSystem:
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

            R_TRY(content_storage->Initialize(root->path, content_path_func, placeholder_path_func, delay_flush, &g_rights_id_cache));
            root->content_storage = std::move(content_storage);
        }

        mount_guard.Cancel();
        return ResultSuccess();
    }

    Result InactivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        /* Already inactivated. */
        R_UNLESS(root->content_storage != nullptr, ResultSuccess());

        root->content_storage->DisableForcibly();
        root->content_storage = nullptr;
        fs::Unmount(root->mount_point);
        return ResultSuccess();
    }

    Result CreateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        R_UNLESS(storage_id != StorageId::GameCard, ncm::ResultUnknownStorage());
        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));

        /* N doesn't bother checking the result of this. */
        fsDisableAutoSaveDataCreation();

        R_TRY(EnsureAndMountSystemSaveData(entry->mount_point, entry->save_meta));
        ON_SCOPE_EXIT { fs::Unmount(entry->mount_point); };

        R_TRY(fs::EnsureDirectoryRecursively(entry->meta_path));
        R_TRY(fsdevCommitDevice(entry->mount_point));

        return ResultSuccess();
    }

    Result VerifyContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        R_UNLESS(storage_id != StorageId::GameCard, ResultSuccess());
        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));

        auto mount_guard = SCOPE_GUARD { fs::Unmount(entry->mount_point); };
        if (!entry->content_meta_database) {
            R_TRY(fs::MountSystemSaveData(entry->mount_point, entry->save_meta.space_id, entry->save_meta.id));
        } else {
            mount_guard.Cancel();
        }

        bool has_meta_path = false;
        R_TRY(fs::HasDirectory(&has_meta_path, entry->meta_path));
        R_UNLESS(has_meta_path, ncm::ResultInvalidContentMetaDatabase());

        return ResultSuccess();
    }

    Result OpenContentMetaDatabase(std::shared_ptr<IContentMetaDatabase> *out, StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));
        
        auto content_meta_db = entry->content_meta_database;

        if (hos::GetVersion() >= hos::Version_200) {
            R_UNLESS(content_meta_db, GetContentMetaDatabaseNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content meta databases as soon as they are opened. */
            if (!content_meta_db) {
                R_TRY(ActivateContentMetaDatabase(storage_id));
                content_meta_db = entry->content_meta_database;
            }
        }

        *out = std::move(content_meta_db);
        return ResultSuccess();
    }

    Result CloseContentMetaDatabaseForcibly(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        R_UNLESS(storage_id != StorageId::None, ncm::ResultUnknownStorage());
        ContentMetaDatabaseEntry *entry;
        R_TRY(FindContentMetaDatabaseEntry(&entry, storage_id));
        
        auto content_meta_db = entry->content_meta_database;

        if (content_meta_db) {
            /* N doesn't bother checking the result of this */
            content_meta_db->DisableForcibly();

            if (storage_id != StorageId::GameCard) {
                fs::Unmount(entry->mount_point);
            }

            entry->content_meta_database = nullptr;
            entry->kvs = std::nullopt;
        }

        return ResultSuccess();
    }

    Result CleanupContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));

        R_TRY(fsDeleteSaveDataFileSystemBySaveDataSpaceId(entry->save_meta.space_id, entry->save_meta.id));
        return ResultSuccess();
    }

    Result ActivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));

        /* Already activated. */
        R_UNLESS(entry->content_meta_database == nullptr, ResultSuccess());

        /* Make a brand new kvs. N doesn't quite do this, but we will for cleanliness. */
        entry->kvs.emplace();

        if (storage_id != StorageId::GameCard) {
            R_TRY(fs::MountSystemSaveData(entry->mount_point, entry->save_meta.space_id, entry->save_meta.id));
            auto mount_guard = SCOPE_GUARD { fs::Unmount(entry->mount_point); };
            R_TRY(entry->kvs->Initialize(entry->meta_path, entry->max_content_metas));
            R_TRY(entry->kvs->Load());
            entry->content_meta_database = std::make_shared<ContentMetaDatabaseInterface>(std::addressof(*entry->kvs), entry->mount_point);
            mount_guard.Cancel();
        } else {
            R_TRY(entry->kvs->Initialize(entry->max_content_metas));
            R_TRY(entry->kvs->Load());
            entry->content_meta_database = std::make_shared<OnMemoryContentMetaDatabaseInterface>(std::addressof(*entry->kvs));
        }

        return ResultSuccess();
    }

    Result InactivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(g_mutex);

        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));

        /* Already inactivated. */
        if (entry->content_meta_database != nullptr) {
            entry->content_meta_database->DisableForcibly();
            entry->content_meta_database = nullptr;
            /* This should lead to Index's destructor performing cleanup for us. */
            entry->kvs = std::nullopt;

            if (storage_id != StorageId::GameCard) {
                fs::Unmount(entry->mount_point);
            }
        }

        return ResultSuccess();
    }

    Result InvalidateRightsIdCache() {
        g_rights_id_cache.Invalidate();
        return ResultSuccess();
    }

}
