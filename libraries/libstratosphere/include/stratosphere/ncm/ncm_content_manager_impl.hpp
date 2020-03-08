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
#include <stratosphere/os.hpp>
#include <stratosphere/fs/fs_mount.hpp>
#include <stratosphere/fs/fs_bis.hpp>
#include <stratosphere/fs/fs_content_storage.hpp>
#include <stratosphere/fs/fs_system_save_data.hpp>
#include <stratosphere/ncm/ncm_i_content_manager.hpp>
#include <stratosphere/ncm/ncm_content_manager_config.hpp>
#include <stratosphere/ncm/ncm_content_meta_database.hpp>
#include <stratosphere/ncm/ncm_bounded_map.hpp>
#include <stratosphere/ncm/ncm_rights_id_cache.hpp>
#include <stratosphere/ncm/ncm_content_management_utils.hpp>
#include <stratosphere/ncm/ncm_content_meta_utils.hpp>
#include <stratosphere/kvdb/kvdb_memory_key_value_store.hpp>

namespace ams::ncm {

    struct SystemSaveDataInfo {
        u64 id;
        u64 size;
        u64 journal_size;
        u32 flags;
        fs::SaveDataSpaceId space_id;
    };
    static_assert(std::is_pod<SystemSaveDataInfo>::value);

    class ContentManagerImpl final : public IContentManager {
        private:
            constexpr static size_t MaxContentStorageRoots         = 8;
            constexpr static size_t MaxContentMetaDatabaseRoots    = 8;
        private:
            struct ContentStorageRoot {
                NON_COPYABLE(ContentStorageRoot);
                NON_MOVEABLE(ContentStorageRoot);

                char mount_name[fs::MountNameLengthMax + 1];
                char path[128];
                StorageId storage_id;
                fs::ContentStorageId content_storage_id;
                std::shared_ptr<IContentStorage> content_storage;

                ContentStorageRoot() { /* ... */ }
            };

            struct ContentMetaDatabaseRoot {
                NON_COPYABLE(ContentMetaDatabaseRoot);
                NON_MOVEABLE(ContentMetaDatabaseRoot);

                char mount_name[fs::MountNameLengthMax + 1];
                char path[128];
                StorageId storage_id;
                SystemSaveDataInfo info;
                std::shared_ptr<IContentMetaDatabase> content_meta_database;
                std::optional<kvdb::MemoryKeyValueStore<ContentMetaKey>> kvs;
                u32 max_content_metas;

                ContentMetaDatabaseRoot() { /* ... */ }
            };
        private:
            os::RecursiveMutex mutex;
            bool initialized;
            ContentStorageRoot content_storage_roots[MaxContentStorageRoots];
            ContentMetaDatabaseRoot content_meta_database_roots[MaxContentMetaDatabaseRoots];
            u32 num_content_storage_entries;
            u32 num_content_meta_entries;
            RightsIdCache rights_id_cache;
        public:
            ContentManagerImpl() : initialized(false) { /* ... */ };
            ~ContentManagerImpl();
        public:
            Result Initialize(const ContentManagerConfig &config);
        private:
            /* Helpers. */
            Result GetContentStorageRoot(ContentStorageRoot **out, StorageId id);
            Result GetContentMetaDatabaseRoot(ContentMetaDatabaseRoot **out, StorageId id);

            Result InitializeContentStorageRoot(ContentStorageRoot *out, StorageId storage_id, fs::ContentStorageId content_storage_id);
            Result InitializeGameCardContentStorageRoot(ContentStorageRoot *out);

            Result InitializeContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, StorageId storage_id, const SystemSaveDataInfo &info, size_t max_content_metas);
            Result InitializeGameCardContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, size_t max_content_metas);

            Result BuildContentMetaDatabase(StorageId storage_id);
            Result ImportContentMetaDatabase(StorageId storage_id, bool from_signed_partition);
            Result ImportContentMetaDatabaseImpl(StorageId storage_id, const char *import_mount_name, const char *path);

            Result EnsureAndMountSystemSaveData(const char *mount, const SystemSaveDataInfo &info) const;
        public:
            /* Actual commands. */
            virtual Result CreateContentStorage(StorageId storage_id) override;
            virtual Result CreateContentMetaDatabase(StorageId storage_id) override;
            virtual Result VerifyContentStorage(StorageId storage_id) override;
            virtual Result VerifyContentMetaDatabase(StorageId storage_id) override;
            virtual Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) override;
            virtual Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) override;
            virtual Result CloseContentStorageForcibly(StorageId storage_id) override;
            virtual Result CloseContentMetaDatabaseForcibly(StorageId storage_id) override;
            virtual Result CleanupContentMetaDatabase(StorageId storage_id) override;
            virtual Result ActivateContentStorage(StorageId storage_id) override;
            virtual Result InactivateContentStorage(StorageId storage_id) override;
            virtual Result ActivateContentMetaDatabase(StorageId storage_id) override;
            virtual Result InactivateContentMetaDatabase(StorageId storage_id) override;
            virtual Result InvalidateRightsIdCache() override;
    };

}
