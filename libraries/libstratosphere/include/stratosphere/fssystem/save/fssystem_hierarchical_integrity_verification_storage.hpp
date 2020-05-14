/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_substorage.hpp>
#include <stratosphere/fs/fs_storage_type.hpp>
#include <stratosphere/fssystem/save/fssystem_i_save_file.hpp>
#include <stratosphere/fssystem/save/fssystem_integrity_verification_storage.hpp>
#include <stratosphere/fssystem/save/fssystem_block_cache_buffered_storage.hpp>

namespace ams::fssystem::save {

    struct HierarchicalIntegrityVerificationLevelInformation {
        fs::Int64 offset;
        fs::Int64 size;
        s32 block_order;
        u8 reserved[4];
    };
    static_assert(util::is_pod<HierarchicalIntegrityVerificationLevelInformation>::value);
    static_assert(sizeof(HierarchicalIntegrityVerificationLevelInformation) == 0x18);
    static_assert(alignof(HierarchicalIntegrityVerificationLevelInformation) == 0x4);

    struct HierarchicalIntegrityVerificationInformation {
        u32 max_layers;
        HierarchicalIntegrityVerificationLevelInformation info[IntegrityMaxLayerCount - 1];
        fs::HashSalt seed;

        s64 GetLayeredHashSize() const {
            return this->info[this->max_layers - 2].offset;
        }

        s64 GetDataOffset() const {
            return this->info[this->max_layers - 2].offset;
        }

        s64 GetDataSize() const {
            return this->info[this->max_layers - 2].size;
        }
    };
    static_assert(util::is_pod<HierarchicalIntegrityVerificationInformation>::value);

    struct HierarchicalIntegrityVerificationMetaInformation {
        u32 magic;
        u32 version;
        u32 master_hash_size;
        HierarchicalIntegrityVerificationInformation level_hash_info;

        /* TODO: Format */
    };
    static_assert(util::is_pod<HierarchicalIntegrityVerificationMetaInformation>::value);

    struct HierarchicalIntegrityVerificationSizeSet {
        s64 control_size;
        s64 master_hash_size;
        s64 layered_hash_sizes[IntegrityMaxLayerCount - 1];
    };
    static_assert(util::is_pod<HierarchicalIntegrityVerificationSizeSet>::value);

    class HierarchicalIntegrityVerificationStorageControlArea {
        NON_COPYABLE(HierarchicalIntegrityVerificationStorageControlArea);
        NON_MOVEABLE(HierarchicalIntegrityVerificationStorageControlArea);
        public:
            static constexpr size_t HashSize = crypto::Sha256Generator::HashSize;

            struct InputParam {
                size_t level_block_size[IntegrityMaxLayerCount - 1];
            };
            static_assert(util::is_pod<InputParam>::value);
        private:
            fs::SubStorage storage;
            HierarchicalIntegrityVerificationMetaInformation meta;
        public:
            static Result QuerySize(HierarchicalIntegrityVerificationSizeSet *out, const InputParam &input_param, s32 layer_count, s64 data_size);
            /* TODO Format */
            static Result Expand(fs::SubStorage meta_storage, const HierarchicalIntegrityVerificationMetaInformation &meta);
        public:
            HierarchicalIntegrityVerificationStorageControlArea() { /* ... */ }

            Result Initialize(fs::SubStorage meta_storage);
            void Finalize();

            u32 GetMasterHashSize() const { return this->meta.master_hash_size; }
            void GetLevelHashInfo(HierarchicalIntegrityVerificationInformation *out) {
                AMS_ASSERT(out != nullptr);
                *out = this->meta.level_hash_info;
            }
    };

    class HierarchicalIntegrityVerificationStorage : public ::ams::fs::IStorage {
        NON_COPYABLE(HierarchicalIntegrityVerificationStorage);
        NON_MOVEABLE(HierarchicalIntegrityVerificationStorage);
        private:
            friend class HierarchicalIntegrityVerificationMetaInformation;
        protected:
            static constexpr s64 HashSize     = crypto::Sha256Generator::HashSize;
            static constexpr size_t MaxLayers = IntegrityMaxLayerCount;
        public:
            using GenerateRandomFunction = void (*)(void *dst, size_t size);

            class HierarchicalStorageInformation {
                public:
                    enum {
                        MasterStorage = 0,
                        Layer1Storage = 1,
                        Layer2Storage = 2,
                        Layer3Storage = 3,
                        Layer4Storage = 4,
                        Layer5Storage = 5,
                        DataStorage   = 6,
                    };
                private:
                    fs::SubStorage storages[DataStorage + 1];
                public:
                    void SetMasterHashStorage(fs::SubStorage s) { this->storages[MasterStorage] = s; }
                    void SetLayer1HashStorage(fs::SubStorage s) { this->storages[Layer1Storage] = s; }
                    void SetLayer2HashStorage(fs::SubStorage s) { this->storages[Layer2Storage] = s; }
                    void SetLayer3HashStorage(fs::SubStorage s) { this->storages[Layer3Storage] = s; }
                    void SetLayer4HashStorage(fs::SubStorage s) { this->storages[Layer4Storage] = s; }
                    void SetLayer5HashStorage(fs::SubStorage s) { this->storages[Layer5Storage] = s; }
                    void SetDataStorage(fs::SubStorage s)       { this->storages[DataStorage] = s; }

                    fs::SubStorage &operator[](s32 index) {
                        AMS_ASSERT(MasterStorage <= index && index <= DataStorage);
                        return this->storages[index];
                    }
            };
        private:
            static GenerateRandomFunction s_generate_random;

            static void SetGenerateRandomFunction(GenerateRandomFunction func) {
                s_generate_random = func;
            }
        private:
            FileSystemBufferManagerSet *buffers;
            os::Mutex *mutex;
            IntegrityVerificationStorage verify_storages[MaxLayers - 1];
            BlockCacheBufferedStorage    buffer_storages[MaxLayers - 1];
            s64 data_size;
            s32 max_layers;
            bool is_written_for_rollback;
        public:
            HierarchicalIntegrityVerificationStorage() : buffers(nullptr), mutex(nullptr), data_size(-1), is_written_for_rollback(false) { /* ... */ }
            virtual ~HierarchicalIntegrityVerificationStorage() override { this->Finalize(); }

            Result Initialize(const HierarchicalIntegrityVerificationInformation &info, HierarchicalStorageInformation storage, FileSystemBufferManagerSet *bufs, os::Mutex *mtx, fs::StorageType storage_type);
            void Finalize();

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;

            virtual Result SetSize(s64 size) override { return fs::ResultUnsupportedOperationInHierarchicalIntegrityVerificationStorageA(); }
            virtual Result GetSize(s64 *out) override;

            virtual Result Flush() override;

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
            using IStorage::OperateRange;

            Result Commit();
            Result OnRollback();

            bool IsInitialized() const {
                return this->data_size >= 0;
            }

            bool IsWrittenForRollback() const {
                return this->is_written_for_rollback;
            }

            FileSystemBufferManagerSet *GetBuffers() {
                return this->buffers;
            }

            void GetParameters(HierarchicalIntegrityVerificationStorageControlArea::InputParam *out) const {
                AMS_ASSERT(out != nullptr);
                for (auto level = 0; level <= this->max_layers - 2; ++level) {
                    out->level_block_size[level] = static_cast<size_t>(this->verify_storages[level].GetBlockSize());
                }
            }

            s64 GetL1HashVerificationBlockSize() const {
                return this->verify_storages[this->max_layers - 2].GetBlockSize();
            }

            fs::SubStorage GetL1HashStorage() {
                return fs::SubStorage(std::addressof(this->buffer_storages[this->max_layers - 3]), 0, util::DivideUp(this->data_size, this->GetL1HashVerificationBlockSize()));
            }
    };

}
