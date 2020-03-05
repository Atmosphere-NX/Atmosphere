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

        constexpr fs::SystemSaveDataId BuiltInSystemSaveDataId = 0x8000000000000120;
        constexpr u64 BuiltInSystemSaveDataSize                = 0x6c000;
        constexpr u64 BuiltInSystemSaveDataJournalSize         = 0x6c000;
        constexpr u32 BuiltInSystemSaveDataFlags               = FsSaveDataFlags_KeepAfterResettingSystemSaveData | FsSaveDataFlags_KeepAfterRefurbishment;

        constexpr SystemSaveDataInfo BuiltInSystemSystemSaveDataInfo = {
            .id              = BuiltInSystemSaveDataId,
            .size            = BuiltInSystemSaveDataSize,
            .journal_size    = BuiltInSystemSaveDataJournalSize,
            .flags           = BuiltInSystemSaveDataFlags,
            .space_id        = fs::SaveDataSpaceId::System
        };

        constexpr fs::SystemSaveDataId BuiltInUserSaveDataId = 0x8000000000000121;
        constexpr u64 BuiltInUserSaveDataSize                = 0x29e000;
        constexpr u64 BuiltInUserSaveDataJournalSize         = 0x29e000;
        constexpr u32 BuiltInUserSaveDataFlags               = 0;

        constexpr SystemSaveDataInfo BuiltInUserSystemSaveDataInfo = {
            .id              = BuiltInUserSaveDataId,
            .size            = BuiltInUserSaveDataSize,
            .journal_size    = BuiltInUserSaveDataJournalSize,
            .flags           = BuiltInUserSaveDataFlags,
            .space_id        = fs::SaveDataSpaceId::System
        };

        constexpr fs::SystemSaveDataId SdCardSaveDataId = 0x8000000000000124;
        constexpr u64 SdCardSaveDataSize                = 0xa08000;
        constexpr u64 SdCardSaveDataJournalSize         = 0xa08000;
        constexpr u32 SdCardSaveDataFlags               = 0;

        constexpr SystemSaveDataInfo SdCardSystemSaveDataInfo = {
            .id              = SdCardSaveDataId,
            .size            = SdCardSaveDataSize,
            .journal_size    = SdCardSaveDataJournalSize,
            .flags           = SdCardSaveDataFlags,
            .space_id        = fs::SaveDataSpaceId::SdSystem,
        };

        constexpr size_t MaxBuiltInSystemContentMetaCount = 0x800;
        constexpr size_t MaxBuiltInUserContentMetaCount   = 0x2000;
        constexpr size_t MaxSdCardContentMetaCount        = 0x2000;
        constexpr size_t MaxGameCardContentMetaCount      = 0x800;

        using RootPath = kvdb::BoundedString<32>;

        inline void ReplaceMountName(char *out_path, const char *mount_name, const char *path) {
            std::strcpy(out_path, mount_name);
            std::strcat(out_path, std::strchr(path, ':'));
        }

        Result EnsureBuiltInSystemSaveDataFlags() {
            u32 cur_flags = 0;
            R_TRY(fs::GetSaveDataFlags(std::addressof(cur_flags), BuiltInSystemSaveDataId));
            if (cur_flags != BuiltInSystemSaveDataFlags) {
                R_TRY(fs::SetSaveDataFlags(BuiltInSystemSaveDataId, fs::SaveDataSpaceId::System, BuiltInSystemSaveDataFlags));
            }
            return ResultSuccess();
        }

        ALWAYS_INLINE Result GetContentStorageNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:       return ResultGameCardContentStorageNotActive();
                case StorageId::BuiltInSystem:  return ResultNandSystemContentStorageNotActive();
                case StorageId::BuiltInUser:    return ResultNandUserContentStorageNotActive();
                case StorageId::SdCard:         return ResultSdCardContentStorageNotActive();
                default:                        return ResultUnknownContentStorageNotActive();
            }
        }

        ALWAYS_INLINE Result GetContentMetaDatabaseNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:       return ResultGameCardContentMetaDatabaseNotActive();
                case StorageId::BuiltInSystem:  return ResultNandSystemContentMetaDatabaseNotActive();
                case StorageId::BuiltInUser:    return ResultNandUserContentMetaDatabaseNotActive();
                case StorageId::SdCard:         return ResultSdCardContentMetaDatabaseNotActive();
                default:                        return ResultUnknownContentMetaDatabaseNotActive();
            }
        }
    }

    ContentManagerImpl::~ContentManagerImpl() {
        std::scoped_lock lk(this->mutex);

        for (auto &root : this->content_storage_roots) {
            this->InactivateContentStorage(root.storage_id);
        }

        for (auto &root : this->content_meta_database_roots) {
            this->InactivateContentMetaDatabase(root.storage_id);
        }
    }

    Result ContentManagerImpl::EnsureAndMountSystemSaveData(const char *mount_name, const SystemSaveDataInfo &info) const {
        constexpr u64 OwnerId = 0;

        fs::DisableAutoSaveDataCreation();

        R_TRY_CATCH(fs::MountSystemSaveData(mount_name, info.space_id, info.id)) {
            R_CATCH(fs::ResultTargetNotFound) {
                R_TRY(fs::CreateSystemSaveData(info.space_id, info.id, OwnerId, info.size, info.journal_size, info.flags));
                R_TRY(fs::MountSystemSaveData(mount_name, info.space_id, info.id));
            }
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result ContentManagerImpl::GetContentStorageRoot(ContentStorageRoot **out, StorageId id) {
        R_UNLESS(IsUniqueStorage(id), ncm::ResultUnknownStorage());

        for (auto &root : this->content_storage_roots) {
            if (root.storage_id == id) {
                *out = std::addressof(root);
                return ResultSuccess();
            }
        }

        return ncm::ResultUnknownStorage();
    }

    Result ContentManagerImpl::GetContentMetaDatabaseRoot(ContentMetaDatabaseRoot **out, StorageId id) {
        R_UNLESS(IsUniqueStorage(id), ncm::ResultUnknownStorage());

        for (auto &root : this->content_meta_database_roots) {
            if (root.storage_id == id) {
                *out = std::addressof(root);
                return ResultSuccess();
            }
        }

        return ncm::ResultUnknownStorage();
    }


    Result ContentManagerImpl::InitializeContentStorageRoot(ContentStorageRoot *out, StorageId storage_id, fs::ContentStorageId content_storage_id) {
        out->storage_id         = storage_id;
        out->content_storage_id = content_storage_id;
        out->content_storage    = nullptr;

        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        std::snprintf(out->path, sizeof(out->path), "%s:/", out->mount_name);

        return ResultSuccess();
    }

    Result ContentManagerImpl::InitializeGameCardContentStorageRoot(ContentStorageRoot *out) {
        out->storage_id         = StorageId::GameCard;
        out->content_storage    = nullptr;

        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        std::snprintf(out->path, sizeof(out->path), "%s:/", out->mount_name);

        return ResultSuccess();
    }

    Result ContentManagerImpl::InitializeContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, StorageId storage_id, const SystemSaveDataInfo &info, size_t max_content_metas) {
        out->storage_id            = storage_id;
        out->info                  = info;
        out->max_content_metas     = max_content_metas;
        out->content_meta_database = nullptr;
        out->kvs                   = std::nullopt;

        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        out->mount_name[0] = '#';
        std::snprintf(out->path, sizeof(out->path), "%s:/meta", out->mount_name);

        return ResultSuccess();
    }

    Result ContentManagerImpl::InitializeGameCardContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, size_t max_content_metas) {
        out->storage_id            = StorageId::GameCard;
        out->max_content_metas     = max_content_metas;
        out->content_meta_database = nullptr;
        out->kvs                   = std::nullopt;

        return ResultSuccess();
    }

    Result ContentManagerImpl::Initialize() {
        std::scoped_lock lk(this->mutex);

        /* Already initialized. */
        R_UNLESS(!this->initialized, ResultSuccess());

        /* Clear storage id for all roots. */
        for (auto &root : this->content_storage_roots) {
            root.storage_id = StorageId::None;
        }

        for (auto &root : this->content_meta_database_roots) {
            root.storage_id = StorageId::None;
        }

        /* First, setup the BuiltInSystem storage entry. */
        R_TRY(this->InitializeContentStorageRoot(&this->content_storage_roots[this->num_content_storage_entries++], StorageId::BuiltInSystem, fs::ContentStorageId::System));
        if (R_FAILED(this->VerifyContentStorage(StorageId::BuiltInSystem))) {
            R_TRY(this->CreateContentStorage(StorageId::BuiltInSystem));
        }
        R_TRY(this->ActivateContentStorage(StorageId::BuiltInSystem));

        /* Next, the BuiltInSystem content meta entry. */
        R_TRY(this->InitializeContentMetaDatabaseRoot(&this->content_meta_database_roots[this->num_content_meta_entries++], StorageId::BuiltInSystem, BuiltInSystemSystemSaveDataInfo, MaxBuiltInSystemContentMetaCount));

        if (R_FAILED(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem))) {
            R_TRY(this->CreateContentMetaDatabase(StorageId::BuiltInSystem));

            /* TODO: N supports building the database depending on config (unused on retail). */
        }

        /* Ensure correct flags on the BuiltInSystem save data. */
        if (hos::GetVersion() >= hos::Version_200) {
            R_TRY(EnsureBuiltInSystemSaveDataFlags());
        }

        R_TRY(this->ActivateContentMetaDatabase(StorageId::BuiltInSystem));

        /* Now for BuiltInUser's content storage and content meta entries. */
        R_TRY(this->InitializeContentStorageRoot(&this->content_storage_roots[this->num_content_storage_entries++], StorageId::BuiltInUser, fs::ContentStorageId::User));
        R_TRY(this->InitializeContentMetaDatabaseRoot(&this->content_meta_database_roots[this->num_content_meta_entries++], StorageId::BuiltInUser, BuiltInUserSystemSaveDataInfo, MaxBuiltInUserContentMetaCount));

        /* Beyond this point, N uses hardcoded indices. */

        /* Next SdCard's content storage and content meta entries. */
        R_TRY(this->InitializeContentStorageRoot(&this->content_storage_roots[2], StorageId::SdCard, fs::ContentStorageId::SdCard));
        R_TRY(this->InitializeContentMetaDatabaseRoot(&this->content_meta_database_roots[2], StorageId::SdCard, SdCardSystemSaveDataInfo, MaxSdCardContentMetaCount));

        /* GameCard's content storage and content meta entries. */
        /* N doesn't set a content storage id for game cards, so we'll just use 0 (System). */
        R_TRY(this->InitializeGameCardContentStorageRoot(&this->content_storage_roots[3]));
        R_TRY(this->InitializeGameCardContentMetaDatabaseRoot(&this->content_meta_database_roots[3], MaxGameCardContentMetaCount));

        this->initialized = true;
        return ResultSuccess();
    }

    Result ContentManagerImpl::CreateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        R_TRY(fs::MountContentStorage(root->mount_name, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_name); };

        R_TRY(impl::EnsureDirectoryRecursively(root->path));
        R_TRY(ContentStorageImpl::InitializeBase(root->path));

        return ResultSuccess();
    }

    Result ContentManagerImpl::CreateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        R_UNLESS(storage_id != StorageId::GameCard, ncm::ResultUnknownStorage());

        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        R_TRY(this->EnsureAndMountSystemSaveData(root->mount_name, root->info));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_name); };

        R_TRY(impl::EnsureDirectoryRecursively(root->path));

        R_TRY(fs::CommitSaveData(root->mount_name));

        return ResultSuccess();
    }

    Result ContentManagerImpl::VerifyContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        char path[0x80];
        auto mount_name = impl::CreateUniqueMountName();  /* should this be fs::? should it be ncm::? ncm::impl? */
        ReplaceMountName(path, mount_name.str, root->path);

        R_TRY(fs::MountContentStorage(mount_name.str, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(mount_name.str); };

        R_TRY(ContentStorageImpl::VerifyBase(path));

        return ResultSuccess();
    }

    Result ContentManagerImpl::VerifyContentMetaDatabase(StorageId storage_id) {
        R_UNLESS(storage_id != StorageId::GameCard, ResultSuccess());

        std::scoped_lock lk(this->mutex);

        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        auto mount_guard = SCOPE_GUARD { fs::Unmount(root->mount_name); };
        if (!root->content_meta_database) {
            R_TRY(fs::MountSystemSaveData(root->mount_name, root->info.space_id, root->info.id));
        } else {
            mount_guard.Cancel();
        }

        bool has_dir = false;
        R_TRY(impl::HasDirectory(&has_dir, root->path));
        R_UNLESS(has_dir, ncm::ResultInvalidContentMetaDatabase());

        return ResultSuccess();
    }

    Result ContentManagerImpl::OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        if (hos::GetVersion() >= hos::Version_200) {
            R_UNLESS(root->content_storage, GetContentStorageNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content storages as soon as they are opened. */
            if (!root->content_storage) {
                R_TRY(this->ActivateContentStorage(storage_id));
            }
        }

        auto content_storage = root->content_storage;
        out.SetValue(std::move(content_storage));
        return ResultSuccess();
    }

    Result ContentManagerImpl::OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));


        if (hos::GetVersion() >= hos::Version_200) {
            R_UNLESS(root->content_meta_database, GetContentMetaDatabaseNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content meta databases as soon as they are opened. */
            if (!root->content_meta_database) {
                R_TRY(this->ActivateContentMetaDatabase(storage_id));
            }
        }

        auto content_meta_db = root->content_meta_database;
        out.SetValue(std::move(content_meta_db));
        return ResultSuccess();
    }

    Result ContentManagerImpl::CloseContentStorageForcibly(StorageId storage_id) {
        return this->InactivateContentStorage(storage_id);
    }

    Result ContentManagerImpl::CloseContentMetaDatabaseForcibly(StorageId storage_id) {
        return this->InactivateContentMetaDatabase(storage_id);
    }

    Result ContentManagerImpl::CleanupContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        R_TRY(this->InactivateContentMetaDatabase(storage_id));

        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        R_TRY(fs::DeleteSaveData(root->info.space_id, root->info.id));
        return ResultSuccess();
    }

    Result ContentManagerImpl::ActivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        /* Check if the storage is already activated. */
        R_UNLESS(root->content_storage == nullptr, ResultSuccess());

        if (storage_id == StorageId::GameCard) {
            fs::GameCardHandle handle;
            R_TRY(fs::GetGameCardHandle(std::addressof(handle)));
            R_TRY(fs::MountGameCardPartition(root->mount_name, handle, fs::GameCardPartition::Secure));
        } else {
            R_TRY(fs::MountContentStorage(root->mount_name, root->content_storage_id));
        }

        auto mount_guard = SCOPE_GUARD { fs::Unmount(root->mount_name); };

        if (storage_id == StorageId::GameCard) {
            auto content_storage = std::make_shared<ReadOnlyContentStorageImpl>();
            R_TRY(content_storage->Initialize(root->path, MakeFlatContentFilePath));
            root->content_storage = std::move(content_storage);
        } else {
            auto content_storage = std::make_shared<ContentStorageImpl>();

            switch (storage_id) {
                case StorageId::BuiltInSystem:
                    R_TRY(content_storage->Initialize(root->path, MakeFlatContentFilePath, MakeFlatPlaceHolderFilePath, false, std::addressof(this->rights_id_cache)));
                    break;
                case StorageId::SdCard:
                    R_TRY(content_storage->Initialize(root->path, MakeSha256HierarchicalContentFilePath_ForFat16KCluster, MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster, true, std::addressof(this->rights_id_cache)));
                    break;
                default:
                    R_TRY(content_storage->Initialize(root->path, MakeSha256HierarchicalContentFilePath_ForFat16KCluster, MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster, false, std::addressof(this->rights_id_cache)));
                    break;
            }

            root->content_storage = std::move(content_storage);
        }

        mount_guard.Cancel();
        return ResultSuccess();
    }

    Result ContentManagerImpl::InactivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        if (root->content_storage) {
            /* N doesn't bother checking the result of this */
            root->content_storage->DisableForcibly();
            root->content_storage = nullptr;
            fs::Unmount(root->mount_name);
        }

        return ResultSuccess();
    }

    Result ContentManagerImpl::ActivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        /* Already activated. */
        R_UNLESS(root->content_meta_database == nullptr, ResultSuccess());

        /* Make a new kvs. */
        root->kvs.emplace();

        if (storage_id == StorageId::GameCard) {
            R_TRY(root->kvs->Initialize(root->max_content_metas));
            root->content_meta_database = std::make_shared<OnMemoryContentMetaDatabaseImpl>(std::addressof(*root->kvs));
        } else {
            R_TRY(fs::MountSystemSaveData(root->mount_name, root->info.space_id, root->info.id));
            auto mount_guard = SCOPE_GUARD { fs::Unmount(root->mount_name); };

            R_TRY(root->kvs->Initialize(root->path, root->max_content_metas));
            R_TRY(root->kvs->Load());

            root->content_meta_database = std::make_shared<ContentMetaDatabaseImpl>(std::addressof(*root->kvs), root->mount_name);
            mount_guard.Cancel();
        }

        return ResultSuccess();
    }

    Result ContentManagerImpl::InactivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        if (root->content_meta_database) {
            /* N doesn't bother checking the result of this */
            root->content_meta_database->DisableForcibly();
            root->content_meta_database = nullptr;
            root->kvs = std::nullopt;

            if (storage_id != StorageId::GameCard) {
                fs::Unmount(root->mount_name);
            }
        }

        return ResultSuccess();
    }

    Result ContentManagerImpl::InvalidateRightsIdCache() {
        this->rights_id_cache.Invalidate();
        return ResultSuccess();
    }

}
