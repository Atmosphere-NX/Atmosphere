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
#include "ncm_content_storage_impl.hpp"
#include "ncm_read_only_content_storage_impl.hpp"
#include "ncm_host_content_storage_impl.hpp"
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
            R_SUCCEED();
        }

        ALWAYS_INLINE Result GetContentStorageNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:       R_THROW(ncm::ResultGameCardContentStorageNotActive());
                case StorageId::BuiltInSystem:  R_THROW(ncm::ResultBuiltInSystemContentStorageNotActive());
                case StorageId::BuiltInUser:    R_THROW(ncm::ResultBuiltInUserContentStorageNotActive());
                case StorageId::SdCard:         R_THROW(ncm::ResultSdCardContentStorageNotActive());
                default:                        R_THROW(ncm::ResultUnknownContentStorageNotActive());
            }
        }

        ALWAYS_INLINE Result GetContentMetaDatabaseNotActiveResult(StorageId storage_id) {
            switch (storage_id) {
                case StorageId::GameCard:       R_THROW(ncm::ResultGameCardContentMetaDatabaseNotActive());
                case StorageId::BuiltInSystem:  R_THROW(ncm::ResultBuiltInSystemContentMetaDatabaseNotActive());
                case StorageId::BuiltInUser:    R_THROW(ncm::ResultBuiltInUserContentMetaDatabaseNotActive());
                case StorageId::SdCard:         R_THROW(ncm::ResultSdCardContentMetaDatabaseNotActive());
                default:                        R_THROW(ncm::ResultUnknownContentMetaDatabaseNotActive());
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
                util::SNPrintf(path, sizeof(path), "%s:/", bis_mount_name);
                return fs::IsSignedSystemPartitionOnSdCardValid(path);
            } else {
                /* On 4.0.0-7.0.1, use the remote command to validate the system partition. */
                return fs::IsSignedSystemPartitionOnSdCardValidDeprecated();
            }
        }

        Result EnsureAndMountSystemSaveData(const char *mount_name, const SystemSaveDataInfo &info) {
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

            R_SUCCEED();
        }

    }

    Result ContentManagerImpl::IntegratedContentStorageRoot::Create() {
        /* Create all storages. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            const auto &root = m_roots[i];

            /* If we should, skip. */
            if (!root.config.has_value() || root.config->skip_verify_and_create) {
                continue;
            }

            /* Mount the relevant content storage. */
            R_TRY(fs::MountContentStorage(root.mount_name, root.config->content_storage_id));
            ON_SCOPE_EXIT { fs::Unmount(root.mount_name); };

            /* Ensure the content storage root's path exists. */
            R_TRY(fs::EnsureDirectory(root.path));

            /* Initialize content and placeholder directories for the root. */
            R_TRY(ContentStorageImpl::InitializeBase(root.path));
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentStorageRoot::Verify() {
        /* Verify all storages. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            const auto &root = m_roots[i];

            /* If we should, skip. */
            if (!root.config.has_value() || root.config->skip_verify_and_create) {
                continue;
            }

            /* Substitute the mount name in the root's path with a unique one. */
            char path[0x80];
            auto mount_name = impl::CreateUniqueMountName();
            ReplaceMountName(path, mount_name.str, root.path);

            /* Mount the relevant content storage. */
            R_TRY(fs::MountContentStorage(mount_name.str, root.config->content_storage_id));
            ON_SCOPE_EXIT { fs::Unmount(mount_name.str); };

            /* Ensure the root, content and placeholder directories exist for the storage. */
            R_TRY(ContentStorageImpl::VerifyBase(path));
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentStorageRoot::Open(sf::Out<sf::SharedPointer<IContentStorage>> out, RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content) {
        /* Get the interface. */
        const bool has_intf = m_config->is_integrated ? m_integrated_content_storage != nullptr : m_roots->content_storage != nullptr;

        if (hos::GetVersion() >= hos::Version_2_0_0) {
            /* Obtain the content storage if already active. */
            R_UNLESS(has_intf, GetContentStorageNotActiveResult(m_config->storage_id));
        } else {
            /* 1.0.0 activates content storages as soon as they are opened. */
            if (!has_intf) {
                R_TRY(this->Activate(rights_id_cache, registered_host_content));
            }
        }

        *out = m_config->is_integrated ? m_integrated_content_storage : m_roots->content_storage;
        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentStorageRoot::Activate(RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content) {
        /* If necessary, create the integrated storage. */
        if (m_config->is_integrated && !m_integrated_content_storage) {
            m_integrated_content_storage = sf::CreateSharedObjectEmplaced<IContentStorage, IntegratedContentStorageImpl>();
        }

        /* Activate all storages. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            auto &root = m_roots[i];

            /* If we should, skip. */
            if (root.config.has_value() && root.config->skip_activate) {
                continue;
            }

            /* Activate. */
            R_TRY(this->Activate(root, rights_id_cache, registered_host_content));

            /* If we're integrated, add the storage. */
            if (m_config->is_integrated) {
                /* Determine the index of the storage. */
                int index;
                for (index = 0; index < m_config->num_content_storage_ids; ++index) {
                    if (m_config->content_storage_ids[index] == root.config->content_storage_id) {
                        break;
                    }
                }

                /* Add the storage. */
                m_integrated_content_storage.GetImpl().Add(root.content_storage, index + 1);
            }
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentStorageRoot::Inactivate(RegisteredHostContent &registered_host_content) {
        /* If we have an integrated storage, disable it. */
        if (m_integrated_content_storage != nullptr) {
            R_TRY(m_integrated_content_storage->DisableForcibly());
            m_integrated_content_storage.Reset();
        }

        /* Disable and unmount all storages. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            auto &root = m_roots[i];

            if (root.content_storage != nullptr) {
                /* N doesn't bother checking the result of this. */
                root.content_storage->DisableForcibly();
                root.content_storage = nullptr;

                if (root.storage_id == StorageId::Host) {
                    registered_host_content.ClearPaths();
                } else {
                    fs::Unmount(root.mount_name);
                }
            }
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentStorageRoot::Activate(RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content, fs::ContentStorageId content_storage_id) {
        /* Check that we have the desired storage id in some root. */
        ContentStorageRoot *root = this->GetRoot(content_storage_id);
        R_UNLESS(root != nullptr, ncm::ResultUnknownStorage());

        /* If necessary, create the integrated storage. */
        if (m_config->is_integrated && !m_integrated_content_storage) {
            m_integrated_content_storage = sf::CreateSharedObjectEmplaced<IContentStorage, IntegratedContentStorageImpl>();
        }

        /* Activate. */
        R_TRY(this->Activate(*root, rights_id_cache, registered_host_content));

        /* If we're integrated, add the storage. */
        if (m_config->is_integrated) {
            /* Determine the index of the storage. */
            int index;
            for (index = 0; index < m_config->num_content_storage_ids; ++index) {
                if (m_config->content_storage_ids[index] == root->config->content_storage_id) {
                    break;
                }
            }

            /* Add the storage. */
            m_integrated_content_storage.GetImpl().Add(root->content_storage, index + 1);
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentStorageRoot::Activate(ContentStorageRoot &root, RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content) {
        /* Check if the storage is already activated. */
        R_SUCCEED_IF(root.content_storage != nullptr);

        /* Handle based on whether or not we have a content storage config. */
        if (root.config.has_value()) {
            /* Mount the content storage. */
            R_TRY(fs::MountContentStorage(root.mount_name, root.config->content_storage_id));
            ON_RESULT_FAILURE { fs::Unmount(root.mount_name); };

            /* Create a content storage. */
            auto content_storage = sf::CreateSharedObjectEmplaced<IContentStorage, ContentStorageImpl>();

            /* Initialize content storage with an appropriate path function. */
            switch (root.storage_id) {
                case StorageId::BuiltInSystem:
                    R_TRY(content_storage.GetImpl().Initialize(root.path, MakeFlatContentFilePath, MakeFlatPlaceHolderFilePath, false, std::addressof(rights_id_cache)));
                    break;
                case StorageId::SdCard:
                    R_TRY(content_storage.GetImpl().Initialize(root.path, MakeSha256HierarchicalContentFilePath_ForFat16KCluster, MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster, true, std::addressof(rights_id_cache)));
                    break;
                default:
                    R_TRY(content_storage.GetImpl().Initialize(root.path, MakeSha256HierarchicalContentFilePath_ForFat16KCluster, MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster, false, std::addressof(rights_id_cache)));
                    break;
            }

            root.content_storage = std::move(content_storage);
        } else {
            switch (root.storage_id) {
                case ncm::StorageId::Host:
                    root.content_storage = sf::CreateSharedObjectEmplaced<IContentStorage, HostContentStorageImpl>(std::addressof(registered_host_content));
                    break;
                case ncm::StorageId::GameCard:
                    {
                        /* Get the gamecard handle. */
                        fs::GameCardHandle handle{};
                        R_TRY(fs::GetGameCardHandle(std::addressof(handle)));

                        /* Mount the secure gamecard partition. */
                        R_TRY(fs::MountGameCardPartition(root.mount_name, handle, fs::GameCardPartition::Secure));
                        ON_RESULT_FAILURE { fs::Unmount(root.mount_name); };

                        /* Create the content storage. */
                        auto content_storage = sf::CreateSharedObjectEmplaced<IContentStorage, ReadOnlyContentStorageImpl>();
                        R_TRY(content_storage.GetImpl().Initialize(root.path, MakeFlatContentFilePath));
                        root.content_storage = std::move(content_storage);
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Create() {
        /* Create all content meta databases. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            const auto &root = m_roots[i];

            /* If we should, skip. */
            if (!root.save_data_info.has_value()) {
                continue;
            }

            /* Mount (and optionally create) save data for the root. */
            R_TRY(EnsureAndMountSystemSaveData(root.mount_name, *root.save_data_info));
            ON_SCOPE_EXIT { fs::Unmount(root.mount_name); };

            /* Ensure the content meta database root's path exists. */
            R_TRY(fs::EnsureDirectory(root.path));

            /* Commit our changes. */
            R_TRY(fs::CommitSaveData(root.mount_name));
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Verify() {
        /* Verify all content meta databases. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            const auto &root = m_roots[i];

            /* If we should, skip. */
            if (!root.save_data_info.has_value()) {
                continue;
            }

            /* Mount save data for non-existing content meta databases. */
            const bool mount = !root.content_meta_database;
            if (mount) {
                R_TRY(fs::MountSystemSaveData(root.mount_name, root.save_data_info->space_id, root.save_data_info->id));
            }
            auto mount_guard = SCOPE_GUARD { if (mount) { fs::Unmount(root.mount_name); } };

            /* Ensure the root path exists. */
            bool has_dir = false;
            R_TRY(fs::HasDirectory(std::addressof(has_dir), root.path));
            R_UNLESS(has_dir, ncm::ResultInvalidContentMetaDatabase());
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Open(sf::Out<sf::SharedPointer<IContentMetaDatabase>> out) {
        /* Get the interface. */
        const bool has_intf = m_config->is_integrated ? m_integrated_content_meta_database != nullptr : m_roots->content_meta_database != nullptr;

        if (hos::GetVersion() >= hos::Version_2_0_0) {
            /* Obtain the content meta database if already active. */
            R_UNLESS(has_intf, GetContentMetaDatabaseNotActiveResult(m_config->storage_id));
        } else {
            /* 1.0.0 activates content meta database as soon as they are opened. */
            if (!has_intf) {
                R_TRY(this->Activate());
            }
        }

        *out = m_config->is_integrated ? m_integrated_content_meta_database : m_roots->content_meta_database;
        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Cleanup() {
        /* Cleanup all content meta databases. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            const auto &root = m_roots[i];

            /* If we should, skip. */
            if (!root.save_data_info.has_value()) {
                continue;
            }

            /* Delete save data for the content meta database. */
            R_TRY(fs::DeleteSaveData(root.save_data_info->space_id, root.save_data_info->id));
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Activate() {
        /* If necessary, create the integrated meta database. */
        if (m_config->is_integrated && !m_integrated_content_meta_database) {
            m_integrated_content_meta_database = sf::CreateSharedObjectEmplaced<IContentMetaDatabase, IntegratedContentMetaDatabaseImpl>();
        }

        /* Activate all meta databases. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            auto &root = m_roots[i];

            /* If we should, skip. */
            if (root.storage_config.has_value() && root.storage_config->skip_activate) {
                continue;
            }

            /* Activate. */
            R_TRY(this->Activate(root));

            /* If we're integrated, add the meta database. */
            if (m_config->is_integrated) {
                /* Determine the index of the meta database. */
                int index;
                for (index = 0; index < m_config->num_content_storage_ids; ++index) {
                    if (m_config->content_storage_ids[index] == root.storage_config->content_storage_id) {
                        break;
                    }
                }

                /* Add the meta database. */
                m_integrated_content_meta_database.GetImpl().Add(root.content_meta_database, index + 1);
            }
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Inactivate() {
        /* If we have an integrated meta database, disable it. */
        /* NOTE: Nintendo does not reset the integrated DB, presumably they just forgot... */
        /*       ...this breaks this, hard, so we'll do the correct thing. */
        if (m_integrated_content_meta_database != nullptr) {
            R_TRY(m_integrated_content_meta_database->DisableForcibly());
            m_integrated_content_meta_database.Reset();
        }

        /* Disable and unmount all storages. */
        for (auto i = 0; i < m_num_roots; ++i) {
            /* Get the current root. */
            auto &root = m_roots[i];

            if (root.content_meta_database != nullptr) {
                /* N doesn't bother checking the result of this. */
                root.content_meta_database->DisableForcibly();
                root.content_meta_database = nullptr;
                root.kvs = util::nullopt;

                /* Additionally, if we have savedata, unmount. */
                if (root.save_data_info.has_value()) {
                    fs::Unmount(root.mount_name);
                }
            }
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Activate(ContentMetaDatabaseRoot &root) {
        /* If the root is already activated, there's nothing to do. */
        R_SUCCEED_IF(root.content_meta_database != nullptr);

        /* Make a new kvs. */
        root.kvs.emplace();

        /* If we should, mount save data for this route. */
        if (root.save_data_info.has_value()) {
            /* Mount save data for this root. */
            R_TRY(fs::MountSystemSaveData(root.mount_name, root.save_data_info->space_id, root.save_data_info->id));
            ON_RESULT_FAILURE { fs::Unmount(root.mount_name); };

            /* Initialize and load the key value store from the filesystem. */
            R_TRY(root.kvs->Initialize(root.path, root.max_content_metas, root.memory_resource));
            R_TRY(root.kvs->Load());

            /* Create the content meta database. */
            root.content_meta_database = sf::CreateSharedObjectEmplaced<IContentMetaDatabase, ContentMetaDatabaseImpl>(std::addressof(*root.kvs), root.mount_name);
        } else {
            if (root.storage_id == StorageId::BuiltInSystem) {
                /* Create a temporary mount name, and mount the partition. */
                auto tmp_mount_name = impl::CreateUniqueMountName();

                switch (root.storage_config->content_storage_id) {
                    case fs::ContentStorageId::System:
                        R_TRY(fs::MountBis(tmp_mount_name.str, fs::BisPartitionId::System));
                        break;
                    case fs::ContentStorageId::System0:
                        R_TRY(fs::MountBis(tmp_mount_name.str, fs::BisPartitionId::System0));
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
                ON_SCOPE_EXIT { fs::Unmount(tmp_mount_name.str); };

                /* Initialize and load the key value store from the filesystem. */
                char path[fs::EntryNameLengthMax];
                util::SNPrintf(path, sizeof(path), "%s:/cnmtdb.arc", tmp_mount_name.str);
                R_TRY(root.kvs->InitializeForReadOnlyArchiveFile(path, root.max_content_metas, root.memory_resource));
                R_TRY(root.kvs->Load());

                /* Create an on memory content meta database. */
                root.content_meta_database = sf::CreateSharedObjectEmplaced<IContentMetaDatabase, OnMemoryContentMetaDatabaseImpl>(std::addressof(*root.kvs));
            } else {
                /* Initialize the key value store. */
                R_TRY(root.kvs->Initialize(root.max_content_metas, root.memory_resource));

                /* Create an on memory content meta database. */
                root.content_meta_database = sf::CreateSharedObjectEmplaced<IContentMetaDatabase, OnMemoryContentMetaDatabaseImpl>(std::addressof(*root.kvs));
            }
        }

        R_SUCCEED();
    }

    Result ContentManagerImpl::IntegratedContentMetaDatabaseRoot::Activate(fs::ContentStorageId content_storage_id) {
        /* Check that we have the desired storage id in some root. */
        ContentMetaDatabaseRoot *root = this->GetRoot(content_storage_id);
        R_UNLESS(root != nullptr, ncm::ResultUnknownStorage());

        /* If necessary, create the integrated meta database. */
        if (m_config->is_integrated && !m_integrated_content_meta_database) {
            m_integrated_content_meta_database = sf::CreateSharedObjectEmplaced<IContentMetaDatabase, IntegratedContentMetaDatabaseImpl>();
        }

        /* Activate. */
        R_TRY(this->Activate(*root));

        /* If we're integrated, add the storage. */
        if (m_config->is_integrated) {
            /* Determine the index of the meta database. */
            int index;
            for (index = 0; index < m_config->num_content_storage_ids; ++index) {
                if (m_config->content_storage_ids[index] == root->storage_config->content_storage_id) {
                    break;
                }
            }

            /* Add the meta database. */
            m_integrated_content_meta_database.GetImpl().Add(root->content_meta_database, index + 1);
        }

        R_SUCCEED();
    }

    ContentManagerImpl::~ContentManagerImpl() {
        std::scoped_lock lk(m_mutex);

        /* Disable and unmount all content storage roots. */
        for (size_t i = 0; i < m_num_integrated_content_storage_entries; ++i) {
            this->InactivateContentStorage(m_integrated_content_storage_roots[i].m_config->storage_id);
        }

        /* Disable and unmount all content meta database roots. */
        for (size_t i = 0; i < m_num_integrated_content_meta_entries; ++i) {
            this->InactivateContentMetaDatabase(m_integrated_content_meta_database_roots[i].m_config->storage_id);
        }
    }

    Result ContentManagerImpl::GetIntegratedContentStorageConfig(IntegratedContentStorageConfig **out, fs::ContentStorageId content_storage_id) {
        for (size_t i = 0; i < m_num_integrated_configs; ++i) {
            auto &integrated_config = m_integrated_configs[i];
            for (auto n = 0; n < integrated_config.num_content_storage_ids; n++) {
                if (integrated_config.content_storage_ids[n] == content_storage_id) {
                    *out = std::addressof(integrated_config);
                    R_SUCCEED();
                }
            }
        }

        R_THROW(ncm::ResultUnknownStorage());
    }


    Result ContentManagerImpl::GetIntegratedContentStorageRoot(IntegratedContentStorageRoot **out, StorageId id) {
        /* Storage must not be StorageId::Any or StorageId::None. */
        R_UNLESS(IsUniqueStorage(id), ncm::ResultUnknownStorage());

        /* Find a root with a matching storage id. */
        for (size_t i = 0; i < m_num_integrated_content_storage_entries; ++i) {
            if (auto &root = m_integrated_content_storage_roots[i]; root.m_config->storage_id == id) {
                *out = std::addressof(root);
                R_SUCCEED();
            }
        }

        R_THROW(ncm::ResultUnknownStorage());
    }

    Result ContentManagerImpl::GetIntegratedContentMetaDatabaseRoot(IntegratedContentMetaDatabaseRoot **out, StorageId id) {
        /* Storage must not be StorageId::Any or StorageId::None. */
        R_UNLESS(IsUniqueStorage(id), ncm::ResultUnknownStorage());

        /* Find a root with a matching storage id. */
        for (size_t i = 0; i < m_num_integrated_content_meta_entries; ++i) {
            if (auto &root = m_integrated_content_meta_database_roots[i]; root.m_config->storage_id == id) {
                *out = std::addressof(root);
                R_SUCCEED();
            }
        }

        R_THROW(ncm::ResultUnknownStorage());
    }


    Result ContentManagerImpl::InitializeContentStorageRoot(ContentStorageRoot *out, StorageId storage_id, util::optional<ContentStorageConfig> config) {
        out->storage_id         = storage_id;
        out->config             = config;
        out->content_storage    = nullptr;

        /* Create a new mount name and copy it to out. */
        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        util::SNPrintf(out->path, sizeof(out->path), "%s:/", out->mount_name);

        R_SUCCEED();
    }

    Result ContentManagerImpl::InitializeContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, StorageId storage_id, util::optional<ContentStorageConfig> storage_config) {
        out->storage_id     = storage_id;
        out->storage_config = storage_config;

        /* Set the storage-specific info. */
        switch (storage_id) {
            case ncm::StorageId::Host:
                out->save_data_info    = util::nullopt;
                out->max_content_metas = HostMaxContentMetaCount;
                out->memory_resource   = std::addressof(g_sd_and_user_content_meta_memory_resource);
                break;
            case ncm::StorageId::GameCard:
                out->save_data_info    = util::nullopt;
                out->max_content_metas = GameCardMaxContentMetaCount;
                out->memory_resource   = std::addressof(g_gamecard_content_meta_memory_resource);
                break;
            case ncm::StorageId::BuiltInSystem:
                /* If we should, skip save data info. */
                if (storage_config.has_value() && (storage_config->content_storage_id != fs::ContentStorageId::System || storage_config->skip_verify_and_create)) {
                    out->save_data_info = util::nullopt;
                } else {
                    /* Otherwise, use normal save data info. */
                    out->save_data_info = BuiltInSystemSystemSaveDataInfo;
                }

                out->max_content_metas = SystemMaxContentMetaCount;
                out->memory_resource   = std::addressof(g_system_content_meta_memory_resource);
                break;
            case ncm::StorageId::BuiltInUser:
                out->save_data_info    = BuiltInUserSystemSaveDataInfo;
                out->max_content_metas = UserMaxContentMetaCount;
                out->memory_resource   = std::addressof(g_sd_and_user_content_meta_memory_resource);
                break;
            case ncm::StorageId::SdCard:
                out->save_data_info    = SdCardSystemSaveDataInfo;
                out->max_content_metas = SdCardMaxContentMetaCount;
                out->memory_resource   = std::addressof(g_sd_and_user_content_meta_memory_resource);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Clear the kvs. */
        out->kvs = util::nullopt;

        /* Create a new mount name and copy it to out. */
        std::strcpy(out->mount_name, impl::CreateUniqueMountName().str);
        out->mount_name[0] = '#';
        util::SNPrintf(out->path, sizeof(out->path), "%s:/meta", out->mount_name);

        R_SUCCEED();
    }

    Result ContentManagerImpl::InitializeIntegratedContentStorageRoot(IntegratedContentStorageRoot *out, const IntegratedContentStorageConfig *config, size_t root_idx, size_t root_count) {
        /* Set config and roots. */
        out->m_config    = config;
        out->m_roots     = std::addressof(m_content_storage_roots[root_idx]);
        out->m_num_roots = root_count;

        R_SUCCEED();
    }

    Result ContentManagerImpl::InitializeIntegratedContentMetaDatabaseRoot(IntegratedContentMetaDatabaseRoot *out, const IntegratedContentStorageConfig *config, size_t root_idx, size_t root_count) {
        /* Set config and roots. */
        out->m_config    = config;
        out->m_roots     = std::addressof(m_content_meta_database_roots[root_idx]);
        out->m_num_roots = root_count;

        R_SUCCEED();
    }

    Result ContentManagerImpl::ImportContentMetaDatabaseImpl(ContentMetaDatabaseRoot *root, const char *import_mount_name) {
        /* Check that the root is system. */
        AMS_ABORT_UNLESS(root->storage_id == ncm::StorageId::BuiltInSystem);

        std::scoped_lock lk(m_mutex);

        /* Print the savedata path. */
        PathString savedata_db_path;
        savedata_db_path.AssignFormat("%s/%s", root->path, "imkvdb.arc");

        /* Print a path for the mounted partition. */
        PathString bis_db_path;
        bis_db_path.AssignFormat("%s:/%s", import_mount_name, "cnmtdb.arc");

        /* Mount the savedata. */
        R_TRY(fs::MountSystemSaveData(root->mount_name, root->save_data_info->space_id, root->save_data_info->id));
        ON_SCOPE_EXIT { fs::Unmount(root->mount_name); };

        /* Ensure the path exists for us to import to. */
        R_TRY(fs::EnsureDirectory(root->path));

        /* Copy the file from bis to our save. */
        R_TRY(impl::CopyFile(savedata_db_path, bis_db_path));

        /* Commit the import. */
        R_RETURN(fs::CommitSaveData(root->mount_name));
    }

    Result ContentManagerImpl::BuildContentMetaDatabase(StorageId storage_id) {
        if (hos::GetVersion() < hos::Version_5_0_0) {
            /* On < 5.0.0, perform an actual build of the database. */
            R_RETURN(this->BuildContentMetaDatabaseImpl(storage_id));
        } else {
            /* On 5.0.0+, building just performs an import. */
            R_RETURN(this->ImportContentMetaDatabase(storage_id, false));
        }
    }

    Result ContentManagerImpl::BuildContentMetaDatabaseImpl(StorageId storage_id) {
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
        R_RETURN(builder.BuildFromStorage(std::addressof(storage)));
    }

    Result ContentManagerImpl::ImportContentMetaDatabase(StorageId storage_id, bool from_signed_partition) {
        /* Only support importing BuiltInSystem. */
        AMS_ABORT_UNLESS(storage_id == StorageId::BuiltInSystem);

        /* Obtain the integrated content meta database root. */
        IntegratedContentMetaDatabaseRoot *integrated_root;
        R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(integrated_root), storage_id));

        /* Obtain the root. */
        ContentMetaDatabaseRoot *root = integrated_root->GetRoot(fs::ContentStorageId::System);
        R_UNLESS(root != nullptr, ncm::ResultUnknownStorage());

        /* Get a mount name for the system partition. */
        auto bis_mount_name = impl::CreateUniqueMountName();

        /* Mount the BIS partition that contains the database we're importing. */
        R_TRY(fs::MountBis(bis_mount_name.str, fs::BisPartitionId::System));
        ON_SCOPE_EXIT { fs::Unmount(bis_mount_name.str); };

        /* If we're not importing from a signed partition (or the partition signature is valid), import. */
        if (!from_signed_partition || IsSignedSystemPartitionOnSdCardValid(bis_mount_name.str)) {
            R_TRY(this->ImportContentMetaDatabaseImpl(root, bis_mount_name.str));
        }

        R_SUCCEED();
    }

    bool ContentManagerImpl::IsNeedRebuildSystemContentMetaDatabase() {
        /* TODO: Should hos::GetVersion() >= hos::Version_17_0_0 be checked? */

        /* If we do not actually have a content meta db, we should re-build. */
        if (R_FAILED(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem))) {
            return true;
        }

        /* We have a content meta db. Temporarily, activate it. */
        if (R_FAILED(this->ActivateContentMetaDatabase(StorageId::BuiltInSystem))) {
            return true;
        }
        ON_SCOPE_EXIT { this->InactivateContentMetaDatabase(StorageId::BuiltInSystem); };

        /* Open the content meta db. */
        ContentMetaDatabase meta_db;
        R_ABORT_UNLESS(ncm::OpenContentMetaDatabase(std::addressof(meta_db), StorageId::BuiltInSystem));

        /* List the meta db's contents. */
        const auto list_count = meta_db.ListContentMeta(nullptr, 0);

        /* We need to rebuild if the db has zero entries. */
        return list_count.total == 0;
    }

    Result ContentManagerImpl::Initialize(const ContentManagerConfig &config) {
        /* Initialize based on whether integrated content is enabled. */
        if (config.IsIntegratedSystemContentEnabled()) {
            constexpr const IntegratedContentStorageConfig IntegratedConfigsForIntegratedSystemContent[] = {
                { ncm::StorageId::BuiltInSystem, { fs::ContentStorageId::System, fs::ContentStorageId::System0 }, 2, true },
                { ncm::StorageId::BuiltInUser,   { fs::ContentStorageId::User }, 1, false },
                { ncm::StorageId::SdCard,        { fs::ContentStorageId::SdCard }, 1, false },
                { ncm::StorageId::GameCard,      { }, 0, false },
                { ncm::StorageId::Host,          { }, 0, false },
            };
            constexpr const ContentStorageConfig ContentStorageConfigsForIntegratedSystemContent[] = {
                { .content_storage_id = fs::ContentStorageId::System,  .skip_verify_and_create = true,  .skip_activate = true, },
                { .content_storage_id = fs::ContentStorageId::System0, .skip_verify_and_create = true,  .skip_activate = false, },
                { .content_storage_id = fs::ContentStorageId::User,    .skip_verify_and_create = false, .skip_activate = false, },
                { .content_storage_id = fs::ContentStorageId::SdCard,  .skip_verify_and_create = false, .skip_activate = false, },
            };
            constexpr const ncm::StorageId ActivatedStoragesForIntegratedSystemContent[] = {
                ncm::StorageId::BuiltInSystem,
            };

            R_RETURN(this->Initialize(config, IntegratedConfigsForIntegratedSystemContent, util::size(IntegratedConfigsForIntegratedSystemContent), ContentStorageConfigsForIntegratedSystemContent, util::size(ContentStorageConfigsForIntegratedSystemContent), ActivatedStoragesForIntegratedSystemContent, util::size(ActivatedStoragesForIntegratedSystemContent)));
        } else {
            constexpr const IntegratedContentStorageConfig IntegratedConfigs[] = {
                { ncm::StorageId::BuiltInSystem, { fs::ContentStorageId::System }, 1, false },
                { ncm::StorageId::BuiltInUser,   { fs::ContentStorageId::User }, 1, false },
                { ncm::StorageId::SdCard,        { fs::ContentStorageId::SdCard }, 1, false },
                { ncm::StorageId::GameCard,      { }, 0, false },
                { ncm::StorageId::Host,          { }, 0, false },
            };
            constexpr const ContentStorageConfig ContentStorageConfigs[] = {
                { .content_storage_id = fs::ContentStorageId::System,  .skip_verify_and_create = false, .skip_activate = false, },
                { .content_storage_id = fs::ContentStorageId::User,    .skip_verify_and_create = false, .skip_activate = false, },
                { .content_storage_id = fs::ContentStorageId::SdCard,  .skip_verify_and_create = false, .skip_activate = false, },
            };
            constexpr const ncm::StorageId ActivatedStorages[] = {
                ncm::StorageId::BuiltInSystem,
            };

            R_RETURN(this->Initialize(config, IntegratedConfigs, util::size(IntegratedConfigs), ContentStorageConfigs, util::size(ContentStorageConfigs), ActivatedStorages, util::size(ActivatedStorages)));
        }
    }

    Result ContentManagerImpl::Initialize(const ContentManagerConfig &manager_config, const IntegratedContentStorageConfig *integrated_configs, size_t num_integrated_configs, const ContentStorageConfig *configs, size_t num_configs, const ncm::StorageId *activated_storages, size_t num_activated_storages) {
        std::scoped_lock lk(m_mutex);

        /* Check if we've already initialized. */
        R_SUCCEED_IF(m_initialized);

        /* Set our configs. */
        for (size_t i = 0; i < num_integrated_configs; ++i) {
            m_integrated_configs[i] = integrated_configs[i];
        }
        m_num_integrated_configs = num_integrated_configs;

        for (size_t i = 0; i < num_configs; ++i) {
            m_configs[i] = configs[i];
        }
        m_num_configs = num_configs;

        /* Setup roots. */
        m_num_integrated_content_storage_entries = 0;
        m_num_content_storage_entries            = 0;
        m_num_integrated_content_meta_entries    = 0;
        m_num_content_meta_entries               = 0;
        for (size_t i = 0; i < m_num_integrated_configs; ++i) {
            /* Get the integrated config. */
            const auto &integrated_config = m_integrated_configs[i];

            /* Set up storage and meta db roots. */
            size_t content_storage_root_idx = m_num_content_storage_entries;
            size_t content_meta_root_idx    = m_num_content_meta_entries;
            if (integrated_config.num_content_storage_ids > 0) {
                /* If we have content storage ids, set up storage/meta db roots for each. */
                for (auto n = 0; n < integrated_config.num_content_storage_ids; n++) {
                    /* Get the config. */
                    const auto &config = this->GetContentStorageConfig(integrated_config.content_storage_ids[n]);
                    R_TRY(this->InitializeContentStorageRoot(std::addressof(m_content_storage_roots[m_num_content_storage_entries++]), integrated_config.storage_id, config));
                    R_TRY(this->InitializeContentMetaDatabaseRoot(std::addressof(m_content_meta_database_roots[m_num_content_meta_entries++]), integrated_config.storage_id, config));
                }
            } else {
                /* If we have no content storage ids, set up a single storage/meta db root. */
                R_TRY(this->InitializeContentStorageRoot(std::addressof(m_content_storage_roots[m_num_content_storage_entries++]), integrated_config.storage_id, util::nullopt));
                R_TRY(this->InitializeContentMetaDatabaseRoot(std::addressof(m_content_meta_database_roots[m_num_content_meta_entries++]), integrated_config.storage_id, util::nullopt));
            }

            R_TRY(this->InitializeIntegratedContentStorageRoot(std::addressof(m_integrated_content_storage_roots[m_num_integrated_content_storage_entries++]), std::addressof(integrated_config), content_storage_root_idx, m_num_content_storage_entries - content_storage_root_idx));
            R_TRY(this->InitializeIntegratedContentMetaDatabaseRoot(std::addressof(m_integrated_content_meta_database_roots[m_num_integrated_content_meta_entries++]), std::addressof(integrated_config), content_meta_root_idx, m_num_content_meta_entries - content_meta_root_idx));
        }

        /* Activate storages. */
        for (size_t i = 0; i < num_activated_storages; i++) {
            const auto storage_id = activated_storages[i];
            if (storage_id == ncm::StorageId::BuiltInSystem) {
                R_TRY(this->InitializeStorageBuiltInSystem(manager_config));
            } else {
                R_TRY(this->InitializeStorage(storage_id));
            }
        }

        m_initialized = true;
        R_SUCCEED();
    }

    Result ContentManagerImpl::InitializeStorageBuiltInSystem(const ContentManagerConfig &manager_config) {
        /* Setup and activate the storage for BuiltInSystem. */
        if (R_FAILED(this->VerifyContentStorage(StorageId::BuiltInSystem))) {
            R_TRY(this->CreateContentStorage(StorageId::BuiltInSystem));
        }
        R_TRY(this->ActivateContentStorage(StorageId::BuiltInSystem));

        /* NOTE: This logic is unofficial. */
        /* Beginning with 17.0.0+, save management behavior changed. The primary symptom of this is either verify fail */
        /* or an empty kvs, both of which we can fix by performing a rebuild. */
        if (this->IsNeedRebuildSystemContentMetaDatabase()) {
            /* Clean up the system content meta database, to ensure creation can succeed. */
            this->CleanupContentMetaDatabase(StorageId::BuiltInSystem);

            /* Create the content meta database. */
            R_TRY(this->CreateContentMetaDatabase(StorageId::BuiltInSystem));

            /* Rebuild the content meta database. */
            R_TRY(this->BuildContentMetaDatabaseImpl(StorageId::BuiltInSystem));
        }

        /* Setup the content meta database for system. */
        if (R_FAILED(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem))) {
            R_TRY(this->CreateContentMetaDatabase(StorageId::BuiltInSystem));

            /* Try to build or import a database, depending on our configuration. */
            if (manager_config.ShouldBuildDatabase()) {
                /* If we should build the database, do so. */
                R_TRY(this->BuildContentMetaDatabase(StorageId::BuiltInSystem));
                R_TRY(this->VerifyContentMetaDatabase(StorageId::BuiltInSystem));
            } else if (manager_config.ShouldImportDatabaseFromSignedSystemPartitionOnSd()) {
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

        /* Activate the content meta database. */
        R_TRY(this->ActivateContentMetaDatabase(StorageId::BuiltInSystem));

        R_SUCCEED();
    }

    Result ContentManagerImpl::InitializeStorage(ncm::StorageId storage_id) {
        /* Setup and activate the storage. */
        if (R_FAILED(this->VerifyContentStorage(storage_id))) {
            R_TRY(this->CreateContentStorage(storage_id));
        }
        R_TRY(this->ActivateContentStorage(StorageId::BuiltInSystem));

        /* Setup the content meta database for system. */
        if (R_FAILED(this->VerifyContentMetaDatabase(storage_id))) {
            R_TRY(this->CreateContentMetaDatabase(storage_id));
        }
        R_TRY(this->ActivateContentMetaDatabase(storage_id));

        R_SUCCEED();
    }

    Result ContentManagerImpl::CreateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content storage root. */
        IntegratedContentStorageRoot *root;
        R_TRY(this->GetIntegratedContentStorageRoot(std::addressof(root), storage_id));

        R_RETURN(root->Create());
    }

    Result ContentManagerImpl::CreateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content meta database root. */
        IntegratedContentMetaDatabaseRoot *root;
        R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(root), storage_id));

        R_RETURN(root->Create());
    }

    Result ContentManagerImpl::VerifyContentStorage(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content storage root. */
        IntegratedContentStorageRoot *root;
        R_TRY(this->GetIntegratedContentStorageRoot(std::addressof(root), storage_id));

        R_RETURN(root->Verify());
    }

    Result ContentManagerImpl::VerifyContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content meta database root. */
        IntegratedContentMetaDatabaseRoot *root;
        R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(root), storage_id));

        R_RETURN(root->Verify());
    }

    Result ContentManagerImpl::OpenContentStorage(sf::Out<sf::SharedPointer<IContentStorage>> out, StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content storage root. */
        IntegratedContentStorageRoot *root;
        R_TRY(this->GetIntegratedContentStorageRoot(std::addressof(root), storage_id));

        R_RETURN(root->Open(out, m_rights_id_cache, m_registered_host_content));
    }

    Result ContentManagerImpl::OpenContentMetaDatabase(sf::Out<sf::SharedPointer<IContentMetaDatabase>> out, StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content meta database root. */
        IntegratedContentMetaDatabaseRoot *root;
        R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(root), storage_id));

        R_RETURN(root->Open(out));
    }

    Result ContentManagerImpl::CloseContentStorageForcibly(StorageId storage_id) {
        R_RETURN(this->InactivateContentStorage(storage_id));
    }

    Result ContentManagerImpl::CloseContentMetaDatabaseForcibly(StorageId storage_id) {
        R_RETURN(this->InactivateContentMetaDatabase(storage_id));
    }

    Result ContentManagerImpl::CleanupContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content meta database root. */
        IntegratedContentMetaDatabaseRoot *root;
        R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(root), storage_id));

        R_RETURN(root->Cleanup());
    }

    Result ContentManagerImpl::ActivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content storage root. */
        IntegratedContentStorageRoot *root;
        R_TRY(this->GetIntegratedContentStorageRoot(std::addressof(root), storage_id));

        R_RETURN(root->Activate(m_rights_id_cache, m_registered_host_content));
    }

    Result ContentManagerImpl::InactivateContentStorage(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content storage root. */
        IntegratedContentStorageRoot *root;
        R_TRY(this->GetIntegratedContentStorageRoot(std::addressof(root), storage_id));

        R_RETURN(root->Inactivate(m_registered_host_content));
    }

    Result ContentManagerImpl::ActivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content meta database root. */
        IntegratedContentMetaDatabaseRoot *root;
        R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(root), storage_id));

        R_RETURN(root->Activate());
    }

    Result ContentManagerImpl::InactivateContentMetaDatabase(StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the integrated content meta database root. */
        IntegratedContentMetaDatabaseRoot *root;
        R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(root), storage_id));

        R_RETURN(root->Inactivate());
    }

    Result ContentManagerImpl::InvalidateRightsIdCache() {
        m_rights_id_cache.Invalidate();
        R_SUCCEED();
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
        R_SUCCEED();
    }

    Result ContentManagerImpl::ActivateFsContentStorage(fs::ContentStorageId content_storage_id) {
        /* Get the integrated config for the storage. */
        IntegratedContentStorageConfig *integrated_config;
        R_TRY(this->GetIntegratedContentStorageConfig(std::addressof(integrated_config), content_storage_id));

        {
            /* Obtain the integrated content meta database root. */
            IntegratedContentStorageRoot *root;
            R_TRY(this->GetIntegratedContentStorageRoot(std::addressof(root), integrated_config->storage_id));

            R_TRY(root->Activate(m_rights_id_cache, m_registered_host_content, content_storage_id));
        }
        {
            /* Obtain the integrated content meta database root. */
            IntegratedContentMetaDatabaseRoot *root;
            R_TRY(this->GetIntegratedContentMetaDatabaseRoot(std::addressof(root), integrated_config->storage_id));

            R_TRY(root->Activate(content_storage_id));
        }

        R_SUCCEED();
    }

}
