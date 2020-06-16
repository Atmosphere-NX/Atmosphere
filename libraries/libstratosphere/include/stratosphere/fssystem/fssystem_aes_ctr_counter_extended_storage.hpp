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
#include <stratosphere/fssystem/fssystem_aes_ctr_storage.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree.hpp>

namespace ams::fssystem {

    class AesCtrCounterExtendedStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(AesCtrCounterExtendedStorage);
        NON_MOVEABLE(AesCtrCounterExtendedStorage);
        public:
            static constexpr size_t BlockSize = crypto::Aes128CtrEncryptor::BlockSize;
            static constexpr size_t KeySize   = crypto::Aes128CtrEncryptor::KeySize;
            static constexpr size_t IvSize    = crypto::Aes128CtrEncryptor::IvSize;
            static constexpr size_t NodeSize  = 16_KB;

            using IAllocator = BucketTree::IAllocator;

            using DecryptFunction = void(*)(void *dst, size_t dst_size, s32 index, const void *enc_key, size_t enc_key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);

            class IDecryptor {
                public:
                    virtual ~IDecryptor() { /* ... */ }
                    virtual void Decrypt(void *buf, size_t buf_size, const void *enc_key, size_t enc_key_size, void *iv, size_t iv_size) = 0;
                    virtual bool HasExternalDecryptionKey() const = 0;
            };

            struct Entry {
                u8 offset[sizeof(s64)];
                s32 reserved;
                s32 generation;

                void SetOffset(s64 value) {
                    std::memcpy(this->offset, std::addressof(value), sizeof(s64));
                }

                s64 GetOffset() const {
                    s64 value;
                    std::memcpy(std::addressof(value), this->offset, sizeof(s64));
                    return value;
                }
            };
            static_assert(sizeof(Entry)  == 0x10);
            static_assert(alignof(Entry) == 4);
            static_assert(util::is_pod<Entry>::value);
        public:
            static constexpr s64 QueryHeaderStorageSize() {
                return BucketTree::QueryHeaderStorageSize();
            }

            static constexpr s64 QueryNodeStorageSize(s32 entry_count) {
                return BucketTree::QueryNodeStorageSize(NodeSize, sizeof(Entry), entry_count);
            }

            static constexpr s64 QueryEntryStorageSize(s32 entry_count) {
                return BucketTree::QueryEntryStorageSize(NodeSize, sizeof(Entry), entry_count);
            }

            static Result CreateExternalDecryptor(std::unique_ptr<IDecryptor> *out, DecryptFunction func, s32 key_index);
            static Result CreateSoftwareDecryptor(std::unique_ptr<IDecryptor> *out);
        private:
            BucketTree table;
            fs::SubStorage data_storage;
            u8 key[KeySize];
            u32 secure_value;
            s64 counter_offset;
            std::unique_ptr<IDecryptor> decryptor;
        public:
            AesCtrCounterExtendedStorage() : table(), data_storage(), secure_value(), counter_offset(), decryptor() { /* ... */ }
            virtual ~AesCtrCounterExtendedStorage() { this->Finalize(); }

            Result Initialize(IAllocator *allocator, const void *key, size_t key_size, u32 secure_value, s64 counter_offset, fs::SubStorage data_storage, fs::SubStorage node_storage, fs::SubStorage entry_storage, s32 entry_count, std::unique_ptr<IDecryptor> &&decryptor);
            void Finalize();

            bool IsInitialized() const { return this->table.IsInitialized(); }

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;

            virtual Result GetSize(s64 *out) override {
                AMS_ASSERT(out != nullptr);
                *out = this->table.GetSize();
                return ResultSuccess();
            }

            virtual Result Flush() override {
                return ResultSuccess();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                return fs::ResultUnsupportedOperationInAesCtrCounterExtendedStorageA();
            }

            virtual Result SetSize(s64 size) override {
                return fs::ResultUnsupportedOperationInAesCtrCounterExtendedStorageB();
            }
        private:
            Result Initialize(IAllocator *allocator, const void *key, size_t key_size, u32 secure_value, fs::SubStorage data_storage, fs::SubStorage table_storage);
    };

}
