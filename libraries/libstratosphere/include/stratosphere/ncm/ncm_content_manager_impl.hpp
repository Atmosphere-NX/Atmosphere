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

    class ContentMetaMemoryResource : public MemoryResource {
        private:
            mem::StandardAllocator allocator;
            size_t peak_total_alloc_size;
            size_t peak_alloc_size;
        public:
            explicit ContentMetaMemoryResource(void *heap, size_t heap_size) : allocator(heap, heap_size) { /* ... */ }

            mem::StandardAllocator *GetAllocator() { return std::addressof(this->allocator); }
            size_t GetPeakTotalAllocationSize() const { return this->peak_total_alloc_size; }
            size_t GetPeakAllocationSize() const { return this->peak_alloc_size; }
        private:
            virtual void *AllocateImpl(size_t size, size_t alignment) override {
                void *mem = this->allocator.Allocate(size, alignment);
                this->peak_total_alloc_size = std::max(this->allocator.Hash().allocated_size, this->peak_total_alloc_size);
                this->peak_alloc_size = std::max(size, this->peak_alloc_size);
                return mem;
            }

            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                return this->allocator.Free(buffer);
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

    class ContentManagerImpl final {
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
                ContentMetaMemoryResource *memory_resource;
                u32 max_content_metas;

                ContentMetaDatabaseRoot() { /* ... */ }
            };
        private:
            os::Mutex mutex;
            bool initialized;
            ContentStorageRoot content_storage_roots[MaxContentStorageRoots];
            ContentMetaDatabaseRoot content_meta_database_roots[MaxContentMetaDatabaseRoots];
            u32 num_content_storage_entries;
            u32 num_content_meta_entries;
            RightsIdCache rights_id_cache;
        public:
            ContentManagerImpl() : mutex(true), initialized(false) { /* ... */ };
            ~ContentManagerImpl();
        public:
            Result Initialize(const ContentManagerConfig &config);
        private:
            /* Helpers. */
            Result GetContentStorageRoot(ContentStorageRoot **out, StorageId id);
            Result GetContentMetaDatabaseRoot(ContentMetaDatabaseRoot **out, StorageId id);

            Result InitializeContentStorageRoot(ContentStorageRoot *out, StorageId storage_id, fs::ContentStorageId content_storage_id);
            Result InitializeGameCardContentStorageRoot(ContentStorageRoot *out);

            Result InitializeContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, StorageId storage_id, const SystemSaveDataInfo &info, size_t max_content_metas, ContentMetaMemoryResource *mr);
            Result InitializeGameCardContentMetaDatabaseRoot(ContentMetaDatabaseRoot *out, size_t max_content_metas, ContentMetaMemoryResource *mr);

            Result BuildContentMetaDatabase(StorageId storage_id);
            Result ImportContentMetaDatabase(StorageId storage_id, bool from_signed_partition);
            Result ImportContentMetaDatabaseImpl(StorageId storage_id, const char *import_mount_name, const char *path);

            Result EnsureAndMountSystemSaveData(const char *mount, const SystemSaveDataInfo &info) const;
        public:
            /* Actual commands. */
            Result CreateContentStorage(StorageId storage_id);
            Result CreateContentMetaDatabase(StorageId storage_id);
            Result VerifyContentStorage(StorageId storage_id);
            Result VerifyContentMetaDatabase(StorageId storage_id);
            Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id);
            Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id);
            Result CloseContentStorageForcibly(StorageId storage_id);
            Result CloseContentMetaDatabaseForcibly(StorageId storage_id);
            Result CleanupContentMetaDatabase(StorageId storage_id);
            Result ActivateContentStorage(StorageId storage_id);
            Result InactivateContentStorage(StorageId storage_id);
            Result ActivateContentMetaDatabase(StorageId storage_id);
            Result InactivateContentMetaDatabase(StorageId storage_id);
            Result InvalidateRightsIdCache();
            Result GetMemoryReport(sf::Out<MemoryReport> out);
    };
    static_assert(IsIContentManager<ContentManagerImpl>);

}
