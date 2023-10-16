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
#include <stratosphere/ncm/ncm_registered_host_content.hpp>
#include <stratosphere/ncm/ncm_integrated_content_meta_database_impl.hpp>
#include <stratosphere/ncm/ncm_integrated_content_storage_impl.hpp>
#include <stratosphere/kvdb/kvdb_memory_key_value_store.hpp>

namespace ams::ncm {

    class ContentMetaMemoryResource : public MemoryResource {
        private:
            mem::StandardAllocator m_allocator;
            size_t m_peak_total_alloc_size;
            size_t m_peak_alloc_size;
        public:
            explicit ContentMetaMemoryResource(void *heap, size_t heap_size) : m_allocator(heap, heap_size), m_peak_total_alloc_size(0), m_peak_alloc_size(0) { /* ... */ }

            mem::StandardAllocator *GetAllocator() { return std::addressof(m_allocator); }
            size_t GetPeakTotalAllocationSize() const { return m_peak_total_alloc_size; }
            size_t GetPeakAllocationSize() const { return m_peak_alloc_size; }
        private:
            virtual void *AllocateImpl(size_t size, size_t alignment) override {
                void *mem = m_allocator.Allocate(size, alignment);
                m_peak_total_alloc_size = std::max(m_allocator.Hash().allocated_size, m_peak_total_alloc_size);
                m_peak_alloc_size = std::max(size, m_peak_alloc_size);
                return mem;
            }

            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                AMS_UNUSED(size, alignment);
                return m_allocator.Free(buffer);
            }

            virtual bool IsEqualImpl(const MemoryResource &resource) const override {
                return this == std::addressof(resource);
            }
    };

    struct SystemSaveDataInfo {
        u64 id;
        u64 size;
        u64 journal_size;
        u32 flags;
        fs::SaveDataSpaceId space_id;
    };
    static_assert(util::is_pod<SystemSaveDataInfo>::value);

    struct IntegratedContentStorageImpl;

    class ContentManagerImpl {
        private:
            constexpr static size_t MaxContentStorageRoots                = 8;
            constexpr static size_t MaxIntegratedContentStorageRoots      = 8;
            constexpr static size_t MaxContentMetaDatabaseRoots           = 8;
            constexpr static size_t MaxIntegratedContentMetaDatabaseRoots = 8;
            constexpr static size_t MaxConfigs                            = 8;
            constexpr static size_t MaxIntegratedConfigs                  = 8;
        private:
            struct ContentStorageConfig {
                fs::ContentStorageId content_storage_id;
                bool skip_verify_and_create;
                bool skip_activate;
            };

            struct IntegratedContentStorageConfig {
                ncm::StorageId storage_id;
                fs::ContentStorageId content_storage_ids[MaxContentStorageRoots];
                int num_content_storage_ids;
                bool is_integrated;
            };
        private:
            struct ContentStorageRoot {
                NON_COPYABLE(ContentStorageRoot);
                NON_MOVEABLE(ContentStorageRoot);

                char mount_name[fs::MountNameLengthMax + 1];
                char path[128];
                StorageId storage_id;
                util::optional<ContentStorageConfig> config;
                sf::SharedPointer<IContentStorage> content_storage;

                ContentStorageRoot() : mount_name(), path(), storage_id(), config(util::nullopt), content_storage() { /* ... */ }
            };

            struct IntegratedContentStorageRoot {
                NON_COPYABLE(IntegratedContentStorageRoot);
                NON_MOVEABLE(IntegratedContentStorageRoot);

                const IntegratedContentStorageConfig *m_config;
                ContentStorageRoot *m_roots;
                int m_num_roots;
                sf::EmplacedRef<IContentStorage, IntegratedContentStorageImpl> m_integrated_content_storage;

                IntegratedContentStorageRoot() : m_config(), m_roots(), m_num_roots(), m_integrated_content_storage() { /* ... */ }

                Result Create();
                Result Verify();
                Result Open(sf::Out<sf::SharedPointer<IContentStorage>> out, RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content);
                Result Activate(RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content);
                Result Inactivate(RegisteredHostContent &registered_host_content);

                Result Activate(ContentStorageRoot &root, RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content);

                Result Activate(RightsIdCache &rights_id_cache, RegisteredHostContent &registered_host_content, fs::ContentStorageId content_storage_id);

                ContentStorageRoot *GetRoot(fs::ContentStorageId storage_id) {
                    for (auto i = 0; i < m_num_roots; ++i) {
                        if (auto &root = m_roots[i]; root.config.has_value() && root.config->content_storage_id == storage_id) {
                            return std::addressof(root);
                        }
                    }

                    return nullptr;
                }
            };

            struct ContentMetaDatabaseRoot {
                NON_COPYABLE(ContentMetaDatabaseRoot);
                NON_MOVEABLE(ContentMetaDatabaseRoot);

                char mount_name[fs::MountNameLengthMax + 1];
                char path[128];
                StorageId storage_id;
                util::optional<ContentStorageConfig> storage_config;
                util::optional<SystemSaveDataInfo> save_data_info;
                util::optional<kvdb::MemoryKeyValueStore<ContentMetaKey>> kvs;
                sf::SharedPointer<IContentMetaDatabase> content_meta_database;
                ContentMetaMemoryResource *memory_resource;
                u32 max_content_metas;

                ContentMetaDatabaseRoot() : mount_name(), path(), storage_id(), storage_config(util::nullopt), save_data_info(util::nullopt), kvs(util::nullopt), content_meta_database(), memory_resource(), max_content_metas() { /* ... */ }
            };

            struct IntegratedContentMetaDatabaseRoot {
                NON_COPYABLE(IntegratedContentMetaDatabaseRoot);
                NON_MOVEABLE(IntegratedContentMetaDatabaseRoot);

                const IntegratedContentStorageConfig *m_config;
                ContentMetaDatabaseRoot *m_roots;
                int m_num_roots;
                sf::EmplacedRef<IContentMetaDatabase, IntegratedContentMetaDatabaseImpl> m_integrated_content_meta_database;

                IntegratedContentMetaDatabaseRoot() : m_config(), m_roots(), m_num_roots(), m_integrated_content_meta_database() { /* ... */ }

                Result Create();
                Result Verify();
                Result Open(sf::Out<sf::SharedPointer<IContentMetaDatabase>> out);
                Result Cleanup();
                Result Activate();
                Result Inactivate();

                Result Activate(ContentMetaDatabaseRoot &root);

                Result Activate(fs::ContentStorageId content_storage_id);

                ContentMetaDatabaseRoot *GetRoot(fs::ContentStorageId storage_id) {
                    for (auto i = 0; i < m_num_roots; ++i) {
                        if (auto &root = m_roots[i]; root.storage_config.has_value() && root.storage_config->content_storage_id == storage_id) {
                            return std::addressof(root);
                        }
                    }

                    return nullptr;
                }
            };
        private:
            os::SdkRecursiveMutex m_mutex{};
            bool m_initialized{false};
            IntegratedContentStorageRoot m_integrated_content_storage_roots[MaxIntegratedContentStorageRoots]{};
            ContentStorageRoot m_content_storage_roots[MaxContentStorageRoots]{};
            IntegratedContentMetaDatabaseRoot m_integrated_content_meta_database_roots[MaxIntegratedContentMetaDatabaseRoots]{};
            ContentMetaDatabaseRoot m_content_meta_database_roots[MaxContentMetaDatabaseRoots]{};
            IntegratedContentStorageConfig m_integrated_configs[MaxIntegratedConfigs]{};
            ContentStorageConfig m_configs[MaxConfigs]{};
            u32 m_num_integrated_content_storage_entries{0};
            u32 m_num_content_storage_entries{0};
            u32 m_num_integrated_content_meta_entries{0};
            u32 m_num_content_meta_entries{0};
            u32 m_num_integrated_configs{0};
            u32 m_num_configs{0};
            RightsIdCache m_rights_id_cache{};
            RegisteredHostContent m_registered_host_content{};
        public:
            ContentManagerImpl() = default;
            ~ContentManagerImpl();
        public:
            Result Initialize(const ContentManagerConfig &config);
        private:
            Result Initialize(const ContentManagerConfig &manager_config, const IntegratedContentStorageConfig *integrated_configs, size_t num_integrated_configs, const ContentStorageConfig *configs, size_t num_configs, const ncm::StorageId *activated_storages, size_t num_activated_storages);
            Result InitializeStorageBuiltInSystem(const ContentManagerConfig &manager_config);
            Result InitializeStorage(ncm::StorageId storage_id);

            const ContentStorageConfig &GetContentStorageConfig(fs::ContentStorageId content_storage_id) {
                for (size_t i = 0; i < m_num_configs; ++i) {
                    if (m_configs[i].content_storage_id == content_storage_id) {
                        return m_configs[i];
                    }
                }

                /* NOTE: Nintendo accesses out of bounds memory here. Should we explicitly abort? This is guaranteed by data to never happen. */
                AMS_ASSUME(false);
            }
        private:
            /* Helpers. */
            Result GetIntegratedContentStorageConfig(IntegratedContentStorageConfig **out, fs::ContentStorageId content_storage_id);
            Result GetIntegratedContentStorageRoot(IntegratedContentStorageRoot **out, StorageId id);
            Result GetIntegratedContentMetaDatabaseRoot(IntegratedContentMetaDatabaseRoot **out, StorageId id);

            Result InitializeContentStorageRoot(ContentStorageRoot *out, StorageId storage_id, util::optional<ContentStorageConfig> config);
            Result InitializeContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, StorageId storage_id, util::optional<ContentStorageConfig> storage_config);

            Result InitializeIntegratedContentStorageRoot(IntegratedContentStorageRoot *out, const IntegratedContentStorageConfig *config, size_t root_idx, size_t root_count);
            Result InitializeIntegratedContentMetaDatabaseRoot(IntegratedContentMetaDatabaseRoot *out, const IntegratedContentStorageConfig *config, size_t root_idx, size_t root_count);

            Result BuildContentMetaDatabase(StorageId storage_id);
            Result BuildContentMetaDatabaseImpl(StorageId storage_id);
            Result ImportContentMetaDatabase(StorageId storage_id, bool from_signed_partition);
            Result ImportContentMetaDatabaseImpl(ContentMetaDatabaseRoot *root, const char *import_mount_name);
        private:
            /* Helpers for unofficial functionality. */
            bool IsNeedRebuildSystemContentMetaDatabase();
        public:
            /* Actual commands. */
            Result CreateContentStorage(StorageId storage_id);
            Result CreateContentMetaDatabase(StorageId storage_id);
            Result VerifyContentStorage(StorageId storage_id);
            Result VerifyContentMetaDatabase(StorageId storage_id);
            Result OpenContentStorage(sf::Out<sf::SharedPointer<IContentStorage>> out, StorageId storage_id);
            Result OpenContentMetaDatabase(sf::Out<sf::SharedPointer<IContentMetaDatabase>> out, StorageId storage_id);
            Result CloseContentStorageForcibly(StorageId storage_id);
            Result CloseContentMetaDatabaseForcibly(StorageId storage_id);
            Result CleanupContentMetaDatabase(StorageId storage_id);
            Result ActivateContentStorage(StorageId storage_id);
            Result InactivateContentStorage(StorageId storage_id);
            Result ActivateContentMetaDatabase(StorageId storage_id);
            Result InactivateContentMetaDatabase(StorageId storage_id);
            Result InvalidateRightsIdCache();
            Result GetMemoryReport(sf::Out<MemoryReport> out);
            Result ActivateFsContentStorage(fs::ContentStorageId fs_content_storage_id);
    };
    static_assert(IsIContentManager<ContentManagerImpl>);

}
