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
#include "ncm_content_storage_impl.hpp"
#include "ncm_read_only_content_storage_impl.hpp"
#include "ncm_content_meta_database_impl.hpp"
#include "ncm_on_memory_content_meta_database_impl.hpp"
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace {

        alignas(os::MemoryPageSize) u8 g_system_content_meta_database_heap[512_KB];
        alignas(os::MemoryPageSize) u8 g_gamecard_content_meta_database_heap[512_KB];
        alignas(os::MemoryPageSize) u8 g_sd_and_user_content_meta_database_heap[2_MB + 512_KB];

        ContentMetaMemoryResource g_system_content_meta_memory_resource(g_system_content_meta_database_heap, sizeof(g_system_content_meta_database_heap));
        ContentMetaMemoryResource g_gamecard_content_meta_memory_resource(g_gamecard_content_meta_database_heap, sizeof(g_gamecard_content_meta_database_heap));
        ContentMetaMemoryResource g_sd_and_user_content_meta_memory_resource(g_sd_and_user_content_meta_database_heap, sizeof(g_sd_and_user_content_meta_database_heap));

        constexpr fs::SystemSaveDataId BuiltInSystemSaveDataId = 0x8000000000000120;
        constexpr u64 BuiltInSystemSaveDataSize                = 0x6c000;
        constexpr u64 BuiltInSystemSaveDataJournalSize         = 0x6c000;
        constexpr u32 BuiltInSystemSaveDataFlags               = fs::SaveDataFlags_KeepAfterResettingSystemSaveData | fs::SaveDataFlags_KeepAfterRefurbishment;

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

        using RootPath = kvdb::BoundedString<32>;

        inline void ReplaceMountName(char *out_path, const char *mount_name, const char *path) {
            std::strcpy(out_path, mount_name);
            std::strcat(out_path, std::strchr(path, ':'));
        }

        Result EnsureBuiltInSystemSaveDataFlags() {
            u32 cur_flags = 0;

            /* Obtain the existing flags. */
            R_TRY(fs::GetSaveDataFlags(std::addressof(cur_flags), BuiltInSystemSaveDataId));

            /* Update the flags if needed. */
            if (cur_flags != BuiltInSystemSaveDataFlags) {
                R_TRY(fs::SetSaveDataFlags(BuiltInSystemSaveDataId, fs::SaveDataSpaceId::System, BuiltInSystemSaveDataFlags));
            }
            return ResultSuccess();
        }

        ALWAYS_INLINE Result GetContentStorageNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:       return ResultGameCardContentStorageNotActive();
                case StorageId::BuiltInSystem:  return ResultBuiltInSystemContentStorageNotActive();
                case StorageId::BuiltInUser:    return ResultBuiltInUserContentStorageNotActive();
                case StorageId::SdCard:         return ResultSdCardContentStorageNotActive();
                default:                        return ResultUnknownContentStorageNotActive();
            }
        }

        ALWAYS_INLINE Result GetContentMetaDatabaseNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:       return ResultGameCardContentMetaDatabaseNotActive();
                case StorageId::BuiltInSystem:  return ResultBuiltInSystemContentMetaDatabaseNotActive();
                case StorageId::BuiltInUser:    return ResultBuiltInUserContentMetaDatabaseNotActive();
                case StorageId::SdCard:         return ResultSdCardContentMetaDatabaseNotActive();
                default:                        return ResultUnknownContentMetaDatabaseNotActive();
            }
        }

        ALWAYS_INLINE bool IsSignedSystemPartitionOnSdCardValid(const char *bis_mount_name) {
            /* Signed system partition should never be checked on < 4.0.0, as it did not exist before then. */
            AMS_ABORT_UNLESS(hos::GetVersion() >= hos::Version_4_0_0);

            /* If we're importing from system on SD, make sure that the signed system partition is valid. */
            const auto version = hos::GetVersion();
            if (version >= hos::Version_8_0_0) {
                /* On >= 8.0.0, a simpler method was added to check validity. */
                /* This also works on < 4.0.0 (though the system partition will never be on-sd there), */
                /* and so this will always return false. */
                char path[fs::MountNameLengthMax + 2 /* :/ */ + 1];
                std::snprintf(path, sizeof(path), "%s:/", bis_mount_name);
                return fs::IsSignedSystemPartitionOnSdCardValid(path);
            } else {
                /* On 4.0.0-7.0.1, use the remote command to validate the system partition. */
                return fs::IsSignedSystemPartitionOnSdCardValidDeprecated();
            }
        }
    }

    ContentManagerImpl::~ContentManagerImpl() {
        std::scoped_lock lk(this->mutex);

        /* Disable and unmount all content storage roots. */
        for (auto &root : this->content_storage_roots) {
            this->InactivateContentStorage(root.storage_id);
        }

        /* Disable and unmount all content meta database roots. */
        for (auto &root : this->content_meta_database_roots) {
            this->InactivateContentMetaDatabase(root.storage_id);
        }
    }

    Result ContentManagerImpl::EnsureAndMountSystemSaveData(const char *mount_name, const SystemSaveDataInfo &info) const {
        constexpr u64 OwnerId = 0;

        /* Don't create save if absent - We want to handle this case ourselves. */
        fs::DisableAutoSaveDataCreation();

        /* Mount existing system save data if present, otherwise create it then mount. */
        R_TRY_CATCH(fs::MountSystemSaveData(mount_name, info.space_id, info.id)) {
            R_CATCH(fs::ResultTargetNotFound) {
                /* On 1.0.0, not all flags existed. Mask when appropriate. */
                constexpr u32 SaveDataFlags100Mask = fs::SaveDataFlags_KeepAfterResettingSystemSaveData;
                const u32 flags = (hos::GetVersion() >= hos::Version_2_0_0) ? (info.flags) : (info.flags & SaveDataFlags100Mask);
                R_TRY(fs::CreateSystemSaveData(info.space_id, info.id, OwnerId, info.size, info.journal_size, flags));
                R_TRY(fs::MountSystemSaveData(mount_name, info.space_id, info.id));
            }
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result ContentManagerImpl::GetContentStorageRoot(ContentStorageRoot **out, StorageId id) {
        /* Storage must not be StorageId::Any or StorageId::None. */
        R_UNLESS(IsUniqueStorage(id), ncm::ResultUnknownStorage());

        /* Find a root with a matching storage id. */
        for (auto &root : this->content_storage_roots) {
            if (root.storage_id == id) {
                *out = std::addressof(root);
                return ResultSuccess();
            }
        }

        return ncm::ResultUnknownStorage();
    }

    Result ContentManagerImpl::GetContentMetaDatabaseRoot(ContentMetaDatabaseRoot **out, StorageId id) {
        /* Storage must not be StorageId::Any or StorageId::None. */
        R_UNLESS(IsUniqueStorage(id), ncm::ResultUnknownStorage());

        /* Find a root with a matching storage id. */
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

        /* Create a new mount name and copy it to out. */
        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        std::snprintf(out->path, sizeof(out->path), "%s:/", out->mount_name);

        return ResultSuccess();
    }

    Result ContentManagerImpl::InitializeGameCardContentStorageRoot(ContentStorageRoot *out) {
        out->storage_id         = StorageId::GameCard;
        out->content_storage    = nullptr;

        /* Create a new mount name and copy it to out. */
        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        std::snprintf(out->path, sizeof(out->path), "%s:/", out->mount_name);

        return ResultSuccess();
    }

    Result ContentManagerImpl::InitializeContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, StorageId storage_id, const SystemSaveDataInfo &info, size_t max_content_metas, ContentMetaMemoryResource *memory_resource) {
        out->storage_id            = storage_id;
        out->info                  = info;
        out->max_content_metas     = max_content_metas;
        out->memory_resource       = memory_resource;
        out->content_meta_database = nullptr;
        out->kvs                   = std::nullopt;

        /* Create a new mount name and copy it to out. */
        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        out->mount_name[0] = '#';
        std::snprintf(out->path, sizeof(out->path), "%s:/meta", out->mount_name);

        return ResultSuccess();
    }

    Result ContentManagerImpl::InitializeGameCardContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, size_t max_content_metas, ContentMetaMemoryResource *memory_resource) {
        out->storage_id            = StorageId::GameCard;
        out->max_content_metas     = max_content_metas;
        out->memory_resource       = memory_resource;
        out->content_meta_database = nullptr;
        out->kvs                   = std::nullopt;

        return ResultSuccess();
    }

    Result ContentManagerImpl::ImportContentMetaDatabaseImpl(StorageId storage_id, const char *import_mount_name, const char *path) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content meta database root. */
        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        /* Print the savedata path. */
        PathString savedata_db_path;
        savedata_db_path.SetFormat("%s/%s", root->path, "imkvdb.arc");

        /* Print a path for the mounted partition. */
        PathString bis_db_path;
        bis_db_path.SetFormat("%s:/%s", import_mount_name, path);

        /* Mount the savedata. */
        R_TRY(fs::MountSystemSaveData(root->mount_name, root->info.space_id, root->info.id));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_name); };

        /* Ensure the path exists for us to import to. */
        R_TRY(fs::EnsureDirectoryRecursively(root->path));

        /* Copy the file from bis to our save. */
        R_TRY(impl::CopyFile(savedata_db_path, bis_db_path));

        /* Commit the import. */
        return fs::CommitSaveData(root->mount_name);
    }

    Result ContentManagerImpl::BuildContentMetaDatabase(StorageId storage_id) {
        if (hos::GetVersion() < hos::Version_5_0_0) {
            /* Temporarily activate the database. */
            R_TRY(this->ActivateContentMetaDatabase(storage_id));
            ON_SCOPE_EXIT { this->InactivateContentMetaDatabase(storage_id); };

            /* Open the content meta database and storage. */
            ContentMetaDatabase meta_db;
            ContentStorage storage;
            R_TRY(ncm::OpenContentMetaDatabase(std::addressof(meta_db), storage_id));
            R_TRY(ncm::OpenContentStorage(std::addressof(storage), storage_id));

            /* Create a builder, and build. */
            ContentMetaDatabaseBuilder builder(std::addressof(meta_db));
            return builder.BuildFromStorage(std::addressof(storage));
        } else {
            /* On 5.0.0+, building just performs an import. */
            return this->ImportContentMetaDatabase(storage_id, false);
        }
    }

    Result ContentManagerImpl::ImportContentMetaDatabase(StorageId storage_id, bool from_signed_partition) {
        /* Only support importing BuiltInSystem. */
        AMS_ABORT_UNLESS(storage_id == StorageId::BuiltInSystem);

        /* Get a mount name for the system partition. */
        auto bis_mount_name = impl::CreateUniqueMountName();

        /* Mount the BIS partition that contains the database we're importing. */
        R_TRY(fs::MountBis(bis_mount_name.str, fs::BisPartitionId::System));
        ON_SCOPE_EXIT { fs::Unmount(bis_mount_name.str); };

        /* If we're not importing from a signed partition (or the partition signature is valid), import. */
        if (!from_signed_partition || IsSignedSystemPartitionOnSdCardValid(bis_mount_name.str)) {
            R_TRY(this->ImportContentMetaDatabaseImpl(StorageId::BuiltInSystem, bis_mount_name.str, "cnmtdb.arc"));
        }

        return ResultSuccess();
    }

    Result ContentManagerImpl::Initialize(const ContentManagerConfig &config) {
        std::scoped_lock lk(this->mutex);

        /* Check if we've already initialized. */
        R_SUCCEED_IF(this->initialized);

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
        R_TRY(this->InitializeContentMetaDatabaseRoot(&this->content_meta_database_roots[this->num_content_meta_entries++], StorageId::BuiltInSystem, BuiltInSystemSystemSaveDataInfo, SystemMaxContentMetaCount, std::addressof(g_system_content_meta_memory_resource)));

        if (R_FAILED(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem))) {
            R_TRY(this->CreateContentMetaDatabase(StorageId::BuiltInSystem));

            /* Try to build or import a database, depending on our configuration. */
            if (config.ShouldBuildDatabase()) {
                /* If we should build the database, do so. */
                R_TRY(this->BuildContentMetaDatabase(StorageId::BuiltInSystem));
                R_TRY(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem));
            } else if (config.ShouldImportDatabaseFromSignedSystemPartitionOnSd()) {
                /* Otherwise if we should import the database from the SD, do so. */
                R_TRY(this->ImportContentMetaDatabase(StorageId::BuiltInSystem, true));
                R_TRY(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem));
            }
        }

        /* Ensure correct flags on the BuiltInSystem save data. */
        /* NOTE: Nintendo does not check this succeeds, and it does on older system versions. */
        /* We will not check the error, either, even though this kind of defeats the call's purpose. */
        if (hos::GetVersion() >= hos::Version_2_0_0) {
            EnsureBuiltInSystemSaveDataFlags();
        }

        R_TRY(this->ActivateContentMetaDatabase(StorageId::BuiltInSystem));

        /* Now for BuiltInUser's content storage and content meta entries. */
        R_TRY(this->InitializeContentStorageRoot(&this->content_storage_roots[this->num_content_storage_entries++], StorageId::BuiltInUser, fs::ContentStorageId::User));
        R_TRY(this->InitializeContentMetaDatabaseRoot(&this->content_meta_database_roots[this->num_content_meta_entries++], StorageId::BuiltInUser, BuiltInUserSystemSaveDataInfo, UserMaxContentMetaCount, std::addressof(g_sd_and_user_content_meta_memory_resource)));

        /* Beyond this point, N uses hardcoded indices. */

        /* Next SdCard's content storage and content meta entries. */
        R_TRY(this->InitializeContentStorageRoot(&this->content_storage_roots[2], StorageId::SdCard, fs::ContentStorageId::SdCard));
        R_TRY(this->InitializeContentMetaDatabaseRoot(&this->content_meta_database_roots[2], StorageId::SdCard, SdCardSystemSaveDataInfo, SdCardMaxContentMetaCount, std::addressof(g_sd_and_user_content_meta_memory_resource)));

        /* GameCard's content storage and content meta entries. */
        /* N doesn't set a content storage id for game cards, so we'll just use 0 (System). */
        R_TRY(this->InitializeGameCardContentStorageRoot(&this->content_storage_roots[3]));
        R_TRY(this->InitializeGameCardContentMetaDatabaseRoot(&this->content_meta_database_roots[3], GameCardMaxContentMetaCount, std::addressof(g_gamecard_content_meta_memory_resource)));

        this->initialized = true;
        return ResultSuccess();
    }

    Result ContentManagerImpl::CreateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content storage root. */
        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        /* Mount the relevant content storage. */
        R_TRY(fs::MountContentStorage(root->mount_name, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_name); };

        /* Ensure the content storage root's path exists. */
        R_TRY(fs::EnsureDirectoryRecursively(root->path));

        /* Initialize content and placeholder directories for the root. */
        return ContentStorageImpl::InitializeBase(root->path);
    }

    Result ContentManagerImpl::CreateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        R_UNLESS(storage_id != StorageId::GameCard, ncm::ResultUnknownStorage());

        /* Obtain the content meta database root. */
        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        /* Mount (and optionally create) save data for the root. */
        R_TRY(this->EnsureAndMountSystemSaveData(root->mount_name, root->info));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_name); };

        /* Ensure the content meta database root's path exists. */
        R_TRY(fs::EnsureDirectoryRecursively(root->path));

        /* Commit our changes. */
        return fs::CommitSaveData(root->mount_name);
    }

    Result ContentManagerImpl::VerifyContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content storage root. */
        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        /* Substitute the mount name in the root's path with a unique one. */
        char path[0x80];
        auto mount_name = impl::CreateUniqueMountName();
        ReplaceMountName(path, mount_name.str, root->path);

        /* Mount the relevant content storage. */
        R_TRY(fs::MountContentStorage(mount_name.str, root->content_storage_id));
        ON_SCOPE_EXIT { fs::Unmount(mount_name.str); };

        /* Ensure the root, content and placeholder directories exist for the storage. */
        return ContentStorageImpl::VerifyBase(path);
    }

    Result ContentManagerImpl::VerifyContentMetaDatabase(StorageId storage_id) {
        /* Game card content meta databases will always be valid. */
        R_SUCCEED_IF(storage_id == StorageId::GameCard);

        std::scoped_lock lk(this->mutex);

        /* Obtain the content meta database root. */
        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        /* Mount save data for non-existing content meta databases. */
        const bool mount = !root->content_meta_database;
        if (mount) {
            R_TRY(fs::MountSystemSaveData(root->mount_name, root->info.space_id, root->info.id));
        }
        auto mount_guard = SCOPE_GUARD { if (mount) { fs::Unmount(root->mount_name); } };

        /* Ensure the root path exists. */
        bool has_dir = false;
        R_TRY(fs::HasDirectory(&has_dir, root->path));
        R_UNLESS(has_dir, ncm::ResultInvalidContentMetaDatabase());

        return ResultSuccess();
    }

    Result ContentManagerImpl::OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content storage root. */
        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        if (hos::GetVersion() >= hos::Version_2_0_0) {
            /* Obtain the content storage if already active. */
            R_UNLESS(root->content_storage, GetContentStorageNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content storages as soon as they are opened. */
            if (!root->content_storage) {
                R_TRY(this->ActivateContentStorage(storage_id));
            }
        }

        out.SetValue(std::shared_ptr<IContentStorage>(root->content_storage));
        return ResultSuccess();
    }

    Result ContentManagerImpl::OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content meta database root. */
        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        if (hos::GetVersion() >= hos::Version_2_0_0) {
            /* Obtain the content meta database if already active. */
            R_UNLESS(root->content_meta_database, GetContentMetaDatabaseNotActiveResult(storage_id));
        } else {
            /* 1.0.0 activates content meta databases as soon as they are opened. */
            if (!root->content_meta_database) {
                R_TRY(this->ActivateContentMetaDatabase(storage_id));
            }
        }

        out.SetValue(std::shared_ptr<IContentMetaDatabase>(root->content_meta_database));
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

        /* Disable and unmount content meta database root. */
        R_TRY(this->InactivateContentMetaDatabase(storage_id));

        /* Obtain the content meta database root. */
        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        /* Delete save data for the content meta database root. */
        return fs::DeleteSaveData(root->info.space_id, root->info.id);
    }

    Result ContentManagerImpl::ActivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content storage root. */
        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        /* Check if the storage is already activated. */
        R_SUCCEED_IF(root->content_storage != nullptr);

        /* Mount based on the storage type. */
        if (storage_id == StorageId::GameCard) {
            fs::GameCardHandle handle;
            R_TRY(fs::GetGameCardHandle(std::addressof(handle)));
            R_TRY(fs::MountGameCardPartition(root->mount_name, handle, fs::GameCardPartition::Secure));
        } else {
            R_TRY(fs::MountContentStorage(root->mount_name, root->content_storage_id));
        }

        /* Unmount on failure. */
        auto mount_guard = SCOPE_GUARD { fs::Unmount(root->mount_name); };

        if (storage_id == StorageId::GameCard) {
            /* Game card content storage is read only. */
            auto content_storage = sf::MakeShared<IContentStorage, ReadOnlyContentStorageImpl>();
            R_TRY(content_storage->GetImpl().Initialize(root->path, MakeFlatContentFilePath));
            root->content_storage = std::move(content_storage);
        } else {
            /* Create a content storage. */
            auto content_storage = sf::MakeShared<IContentStorage, ContentStorageImpl>();

            /* Initialize content storage with an appropriate path function. */
            switch (storage_id) {
                case StorageId::BuiltInSystem:
                    R_TRY(content_storage->GetImpl().Initialize(root->path, MakeFlatContentFilePath, MakeFlatPlaceHolderFilePath, false, std::addressof(this->rights_id_cache)));
                    break;
                case StorageId::SdCard:
                    R_TRY(content_storage->GetImpl().Initialize(root->path, MakeSha256HierarchicalContentFilePath_ForFat16KCluster, MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster, true, std::addressof(this->rights_id_cache)));
                    break;
                default:
                    R_TRY(content_storage->GetImpl().Initialize(root->path, MakeSha256HierarchicalContentFilePath_ForFat16KCluster, MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster, false, std::addressof(this->rights_id_cache)));
                    break;
            }

            root->content_storage = std::move(content_storage);
        }

        /* Prevent unmounting. */
        mount_guard.Cancel();
        return ResultSuccess();
    }

    Result ContentManagerImpl::InactivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content storage root. */
        ContentStorageRoot *root;
        R_TRY(this->GetContentStorageRoot(std::addressof(root), storage_id));

        /* Disable and unmount the content storage, if present. */
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

        /* Obtain the content meta database root. */
        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        /* Already activated. */
        R_SUCCEED_IF(root->content_meta_database != nullptr);

        /* Make a new kvs. */
        root->kvs.emplace();

        if (storage_id == StorageId::GameCard) {
            /* Initialize the key value store. */
            R_TRY(root->kvs->Initialize(root->max_content_metas, root->memory_resource));

            /* Create an on memory content meta database for game cards. */
            root->content_meta_database = sf::MakeShared<IContentMetaDatabase, OnMemoryContentMetaDatabaseImpl>(std::addressof(*root->kvs));
        } else {
            /* Mount save data for this root. */
            R_TRY(fs::MountSystemSaveData(root->mount_name, root->info.space_id, root->info.id));

            /* Unmount on failure. */
            auto mount_guard = SCOPE_GUARD { fs::Unmount(root->mount_name); };

            /* Initialize and load the key value store from the filesystem. */
            R_TRY(root->kvs->Initialize(root->path, root->max_content_metas, root->memory_resource));
            R_TRY(root->kvs->Load());

            /* Create the content meta database. */
            root->content_meta_database = sf::MakeShared<IContentMetaDatabase, ContentMetaDatabaseImpl>(std::addressof(*root->kvs), root->mount_name);
            mount_guard.Cancel();
        }

        return ResultSuccess();
    }

    Result ContentManagerImpl::InactivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the content meta database root. */
        ContentMetaDatabaseRoot *root;
        R_TRY(this->GetContentMetaDatabaseRoot(&root, storage_id));

        /* Disable the content meta database, if present. */
        if (root->content_meta_database) {
            /* N doesn't bother checking the result of this */
            root->content_meta_database->DisableForcibly();
            root->content_meta_database = nullptr;
            root->kvs = std::nullopt;

            /* Also unmount, except in the case of game cards. */
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

    Result ContentManagerImpl::GetMemoryReport(sf::Out<MemoryReport> out) {
        /* Populate content meta resource states. */
        MemoryReport report = {
            .system_content_meta_resource_state = {
                .peak_total_alloc_size = g_system_content_meta_memory_resource.GetPeakTotalAllocationSize(),
                .peak_alloc_size       = g_system_content_meta_memory_resource.GetPeakAllocationSize(),
                .allocatable_size      = g_system_content_meta_memory_resource.GetAllocator()->GetAllocatableSize(),
                .total_free_size       = g_system_content_meta_memory_resource.GetAllocator()->GetTotalFreeSize(),
            },
            .sd_and_user_content_meta_resource_state {
                .peak_total_alloc_size = g_sd_and_user_content_meta_memory_resource.GetPeakTotalAllocationSize(),
                .peak_alloc_size       = g_sd_and_user_content_meta_memory_resource.GetPeakAllocationSize(),
                .allocatable_size      = g_sd_and_user_content_meta_memory_resource.GetAllocator()->GetAllocatableSize(),
                .total_free_size       = g_sd_and_user_content_meta_memory_resource.GetAllocator()->GetTotalFreeSize(),
            },
            .gamecard_content_meta_resource_state {
                .peak_total_alloc_size = g_gamecard_content_meta_memory_resource.GetPeakTotalAllocationSize(),
                .peak_alloc_size       = g_gamecard_content_meta_memory_resource.GetPeakAllocationSize(),
                .allocatable_size      = g_gamecard_content_meta_memory_resource.GetAllocator()->GetAllocatableSize(),
                .total_free_size       = g_gamecard_content_meta_memory_resource.GetAllocator()->GetTotalFreeSize(),
            },
            .heap_resource_state = {},
        };

        /* Populate heap memory resource state. */
        GetHeapState().GetMemoryResourceState(std::addressof(report.heap_resource_state));

        /* Output the report. */
        out.SetValue(report);
        return ResultSuccess();
    }

}
