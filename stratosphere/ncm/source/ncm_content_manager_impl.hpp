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
#include <stratosphere.hpp>
#include "impl/ncm_rights_cache.hpp"
#include "ncm_fs.hpp"

namespace ams::ncm {

    struct SystemSaveDataInfo {
        u64 id;
        u64 size;
        u64 journal_size;
        u32 flags;
        FsSaveDataSpaceId space_id;
    };

    static_assert(sizeof(SystemSaveDataInfo) == 0x20, "SystemSaveDataInfo definition!");

    class ContentManagerImpl final : public IContentManager {
        private:
            constexpr static size_t MaxContentStorageEntries         = 8;
            constexpr static size_t MaxContentMetaDatabaseEntries    = 8;
        private:
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
                    MountName mount_name = ncm::fs::CreateUniqueMountName();
                    std::strcpy(this->mount_point, mount_name.name);
                    snprintf(this->path, 0x80, "%s:/", this->mount_point);
                }
            };

            struct ContentMetaDatabaseEntry {
                NON_COPYABLE(ContentMetaDatabaseEntry);
                NON_MOVEABLE(ContentMetaDatabaseEntry);

                char mount_point[16];
                char meta_path[128];
                StorageId storage_id;
                SystemSaveDataInfo save_meta;
                std::shared_ptr<IContentMetaDatabase> content_meta_database;
                std::optional<kvdb::MemoryKeyValueStore<ContentMetaKey>> kvs;
                u32 max_content_metas;

                inline ContentMetaDatabaseEntry() : storage_id(StorageId::None), save_meta({0}),
                    content_meta_database(nullptr), kvs(std::nullopt), max_content_metas(0) {
                    mount_point[0] = '\0';
                    meta_path[0] = '\0';
                }

                Result Initialize(StorageId storage_id, const SystemSaveDataInfo& save_meta, size_t max_content_metas) {
                    this->storage_id = storage_id;
                    this->max_content_metas = max_content_metas;
                    this->save_meta = save_meta;
                    this->content_meta_database = nullptr;
                    this->kvs = std::nullopt;
                    MountName mount_name = ncm::fs::CreateUniqueMountName();
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
        private:
            os::Mutex mutex;
            bool initialized = false;
            ContentStorageRoot content_storage_roots[MaxContentStorageEntries];
            ContentMetaDatabaseEntry content_meta_entries[MaxContentMetaDatabaseEntries];
            u32 num_content_storage_entries;
            u32 num_content_meta_entries;
            impl::RightsIdCache rights_id_cache;
        public:
            ContentManagerImpl() { /* ... */ };
            ~ContentManagerImpl();
        public:
            Result Initialize();
        private:
            constexpr inline bool IsUniqueStorage(StorageId id) {
                return id != StorageId::None && id != StorageId::Any;
            }

            Result FindContentStorageRoot(ContentStorageRoot **out, StorageId storage_id) {
                for (size_t i = 0; i < MaxContentStorageEntries; i++) {
                    ContentStorageRoot *root = &this->content_storage_roots[i];

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
                    ContentMetaDatabaseEntry *entry = &this->content_meta_entries[i];

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

            Result EnsureAndMountSystemSaveData(const char *mount_name, const SystemSaveDataInfo &save_meta) {
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
        public:
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
