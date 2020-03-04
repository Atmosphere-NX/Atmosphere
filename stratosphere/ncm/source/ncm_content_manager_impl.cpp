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

#include "ncm_content_manager_impl.hpp"
#include "ncm_content_meta_database_impl.hpp"
#include "ncm_on_memory_content_meta_database_impl.hpp"
#include "ncm_content_storage_impl.hpp"
#include "ncm_make_path.hpp"
#include "ncm_read_only_content_storage_impl.hpp"

namespace ams::ncm {

    namespace {

        constexpr u64 BuiltInSystemSaveDataId             = 0x8000000000000120;
        constexpr u64 BuiltInSystemSaveDataSize           = 0x6c000;
        constexpr u64 BuiltInSystemSaveDataJournalSize    = 0x6c000;
        constexpr u32 BuiltInSystemSaveDataFlags          = FsSaveDataFlags_KeepAfterResettingSystemSaveData | FsSaveDataFlags_KeepAfterRefurbishment;

        constexpr SystemSaveDataInfo BuiltInSystemSystemSaveDataInfo = {
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

        constexpr SystemSaveDataInfo BuiltInUserSystemSaveDataInfo = {
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

        constexpr SystemSaveDataInfo SdCardSystemSaveDataInfo = {
            .id              = SdCardSaveDataId,
            .size            = SdCardSaveDataSize,
            .journal_size    = SdCardSaveDataJournalSize,
            .flags           = SdCardSaveDataFlags,
            .space_id        = FsSaveDataSpaceId_SdSystem,
        };

        constexpr size_t MaxBuiltInSystemContentMetaCount = 0x800;
        constexpr size_t MaxBuiltInUserContentMetaCount   = 0x2000;
        constexpr size_t MaxSdCardContentMetaCount        = 0x2000;

    }

    Result ContentManagerImpl::~ContentManagerImpl() {
        {
            std::scoped_lock lk(this->mutex);

            for (size_t i = 0; i < MaxContentStorageEntries; i++) {
                ContentStorageRoot *entry = &this->content_storage_roots[i];
                this->InactivateContentStorage(entry->storage_id);
            }

            for (size_t i = 0; i < MaxContentMetaDatabaseEntries; i++) {
                ContentMetaDatabaseEntry *entry = &this->content_meta_entries[i];
                this->InactivateContentMetaDatabase(entry->storage_id);
            }
        }

        for (size_t i = 0; i < MaxContentMetaDatabaseEntries; i++) {
            ContentMetaDatabaseEntry *entry = &this->content_meta_entries[i];
            entry->kvs.reset();
        }

        for (size_t i = 0; i < MaxContentStorageEntries; i++) {
            ContentStorageRoot *entry = &this->content_storage_roots[i];
            entry->content_storage = nullptr;
        }
    }

    Result ContentManagerImpl::Initialize() {
        std::scoped_lock lk(this->mutex);

        /* Already initialized. */
        R_UNLESS(!this->initialized, ResultSuccess());

        for (size_t i = 0; i < MaxContentStorageEntries; i++) {
            ContentStorageRoot *entry = &this->content_storage_roots[i];
            entry->storage_id = StorageId::None;
        }

        for (size_t i = 0; i < MaxContentMetaDatabaseEntries; i++) {
            ContentMetaDatabaseEntry *entry = &this->content_meta_entries[i];
            entry->storage_id = StorageId::None;
        }

        /* First, setup the BuiltInSystem storage entry. */
        this->content_storage_roots[this->num_content_storage_entries++].Initialize(StorageId::BuiltInSystem, FsContentStorageId_System);

        if (R_FAILED(this->VerifyContentStorage(StorageId::BuiltInSystem))) {
            R_TRY(this->CreateContentStorage(StorageId::BuiltInSystem));
        }

        R_TRY(this->ActivateContentStorage(StorageId::BuiltInSystem));

        /* Next, the BuiltInSystem content meta entry. */
        R_TRY(this->content_meta_entries[this->num_content_meta_entries++].Initialize(StorageId::BuiltInSystem, BuiltInSystemSystemSaveDataInfo, MaxBuiltInSystemContentMetaCount));

        if (R_FAILED(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem))) {
            R_TRY(this->CreateContentMetaDatabase(StorageId::BuiltInSystem));

            /* TODO: N supports a number of unused modes here, we don't bother implementing them currently. */
        }

        u32 current_flags = 0;
        if (hos::GetVersion() >= hos::Version_200 && R_SUCCEEDED(fs::GetSaveDataFlags(&current_flags, BuiltInSystemSaveDataId)) && current_flags != (FsSaveDataFlags_KeepAfterResettingSystemSaveData | FsSaveDataFlags_KeepAfterRefurbishment)) {
            fs::SetSaveDataFlags(BuiltInSystemSaveDataId, FsSaveDataSpaceId_System, FsSaveDataFlags_KeepAfterResettingSystemSaveData | FsSaveDataFlags_KeepAfterRefurbishment);
        }

        R_TRY(this->ActivateContentMetaDatabase(StorageId::BuiltInSystem));

        /* Now for BuiltInUser's content storage and content meta entries. */
        this->content_storage_roots[this->num_content_storage_entries++].Initialize(StorageId::BuiltInUser, FsContentStorageId_User);
        R_TRY(this->content_meta_entries[this->num_content_meta_entries++].Initialize(StorageId::BuiltInUser, BuiltInUserSystemSaveDataInfo, MaxBuiltInUserContentMetaCount));

        /* Beyond this point N no longer appears to bother */
        /* incrementing the count for content storage entries or content meta entries. */

        /* Next SdCard's content storage and content meta entries. */
        this->content_storage_roots[2].Initialize(StorageId::SdCard, FsContentStorageId_SdCard);
        R_TRY(this->content_meta_entries[2].Initialize(StorageId::SdCard, SdCardSystemSaveDataInfo, MaxSdCardContentMetaCount));

        /* GameCard's content storage and content meta entries. */
        /* N doesn't set a content storage id for game cards, so we'll just use 0 (System). */
        this->content_storage_roots[3].Initialize(StorageId::GameCard, FsContentStorageId_System);
        R_TRY(this->content_meta_entries[3].InitializeGameCard(0x800));

        this->initialized = true;
        return ResultSuccess();
    }

    Result ContentManagerImpl::CreateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        R_TRY(fs::MountContentStorage(root->mount_point, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_point); };

        R_TRY(fs::EnsureDirectoryRecursively(root->path));
        R_TRY(fs::EnsureContentAndPlaceHolderRoot(root->path));

        return ResultSuccess();
    }

    Result ContentManagerImpl::CreateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

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

    Result ContentManagerImpl::VerifyContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        char mount_root[0x80] = {};
        auto mount_name = ncm::fs::CreateUniqueMountName();  /* should this be fs::? should it be ncm::? ncm::impl? */
        ReplaceMountName(mount_root, mount_name.name, root->path);

        R_TRY(fs::MountContentStorage(mount_name.name, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(mount_name.name); };

        R_TRY(fs::CheckContentStorageDirectoriesExist(mount_root));

        return ResultSuccess();
    }

    Result ContentManagerImpl::VerifyContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

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

    Result ContentManagerImpl::OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        auto content_storage = root->content_storage;

        if (hos::GetVersion() >= hos::Version_200) {
            R_UNLESS(content_storage, GetContentStorageNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content storages as soon as they are opened. */
            if (!content_storage) {
                R_TRY(this->ActivateContentStorage(storage_id));
                content_storage = root->content_storage;
            }
        }

        out.SetValue(std::move(content_storage));
        return ResultSuccess();
    }

    Result ContentManagerImpl::OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));

        auto content_meta_db = entry->content_meta_database;

        if (hos::GetVersion() >= hos::Version_200) {
            R_UNLESS(content_meta_db, GetContentMetaDatabaseNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content meta databases as soon as they are opened. */
            if (!content_meta_db) {
                R_TRY(this->ActivateContentMetaDatabase(storage_id));
                content_meta_db = entry->content_meta_database;
            }
        }

        out.SetValue(std::move(content_meta_db));
        return ResultSuccess();
    }

    Result ContentManagerImpl::CloseContentStorageForcibly(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

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

    Result ContentManagerImpl::CloseContentMetaDatabaseForcibly(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

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

    Result ContentManagerImpl::CleanupContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentMetaDatabaseEntry *entry;
        R_TRY(GetUniqueContentMetaDatabaseEntry(&entry, storage_id));

        R_TRY(fsDeleteSaveDataFileSystemBySaveDataSpaceId(entry->save_meta.space_id, entry->save_meta.id));
        return ResultSuccess();
    }

    Result ContentManagerImpl::ActivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

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
            auto content_storage = std::make_shared<ReadOnlyContentStorageImpl>();
            R_TRY(content_storage->Initialize(root->path, path::MakeContentPathFlat));
            root->content_storage = std::move(content_storage);
        } else {
            MakeContentPathFunc content_path_func = nullptr;
            MakePlaceHolderPathFunc placeholder_path_func = nullptr;
            bool delay_flush = false;
            auto content_storage = std::make_shared<ContentStorageImpl>();

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

            R_TRY(content_storage->Initialize(root->path, content_path_func, placeholder_path_func, delay_flush, &this->rights_id_cache));
            root->content_storage = std::move(content_storage);
        }

        mount_guard.Cancel();
        return ResultSuccess();
    }

    Result ContentManagerImpl::InactivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(GetUniqueContentStorageRoot(std::addressof(root), storage_id));

        /* Already inactivated. */
        R_UNLESS(root->content_storage != nullptr, ResultSuccess());

        root->content_storage->DisableForcibly();
        root->content_storage = nullptr;
        fs::Unmount(root->mount_point);
        return ResultSuccess();
    }

    Result ContentManagerImpl::ActivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

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
            entry->content_meta_database = std::make_shared<ContentMetaDatabaseImpl>(std::addressof(*entry->kvs), entry->mount_point);
            mount_guard.Cancel();
        } else {
            R_TRY(entry->kvs->Initialize(entry->max_content_metas));
            R_TRY(entry->kvs->Load());
            entry->content_meta_database = std::make_shared<OnMemoryContentMetaDatabaseImpl>(std::addressof(*entry->kvs));
        }

        return ResultSuccess();
    }

    Result ContentManagerImpl::InactivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

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

    Result ContentManagerImpl::InvalidateRightsIdCache() {
        this->rights_id_cache.Invalidate();
        return ResultSuccess();
    }

}
