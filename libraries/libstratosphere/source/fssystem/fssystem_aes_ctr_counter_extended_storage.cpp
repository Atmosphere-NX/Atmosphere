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
#include <stratosphere.hpp>

namespace ams::fssystem {

    namespace {

        class SoftwareDecryptor final : public AesCtrCounterExtendedStorage::IDecryptor {
            public:
                virtual void Decrypt(void *buf, size_t buf_size, const void *enc_key, size_t enc_key_size, void *iv, size_t iv_size) override final;
                virtual bool HasExternalDecryptionKey() const override final { return false; }
        };

        class ExternalDecryptor final : public AesCtrCounterExtendedStorage::IDecryptor {
            public:
                static constexpr size_t BlockSize = AesCtrCounterExtendedStorage::BlockSize;
                static constexpr size_t KeySize   = AesCtrCounterExtendedStorage::KeySize;
                static constexpr size_t IvSize    = AesCtrCounterExtendedStorage::IvSize;
            private:
                AesCtrCounterExtendedStorage::DecryptFunction decrypt_function;
                s32 key_index;
            public:
                ExternalDecryptor(AesCtrCounterExtendedStorage::DecryptFunction df, s32 key_idx) : decrypt_function(df), key_index(key_idx) {
                    AMS_ASSERT(this->decrypt_function != nullptr);
                }
            public:
                virtual void Decrypt(void *buf, size_t buf_size, const void *enc_key, size_t enc_key_size, void *iv, size_t iv_size) override final;
                virtual bool HasExternalDecryptionKey() const override final { return this->key_index < 0; }
        };

    }

    Result AesCtrCounterExtendedStorage::CreateExternalDecryptor(std::unique_ptr<IDecryptor> *out, DecryptFunction func, s32 key_index) {
        std::unique_ptr<IDecryptor> decryptor = std::make_unique<ExternalDecryptor>(func, key_index);
        R_UNLESS(decryptor != nullptr, fs::ResultAllocationFailureInAesCtrCounterExtendedStorageA());
        *out = std::move(decryptor);
        return ResultSuccess();
    }

    Result AesCtrCounterExtendedStorage::CreateSoftwareDecryptor(std::unique_ptr<IDecryptor> *out) {
        std::unique_ptr<IDecryptor> decryptor = std::make_unique<SoftwareDecryptor>();
        R_UNLESS(decryptor != nullptr, fs::ResultAllocationFailureInAesCtrCounterExtendedStorageA());
        *out = std::move(decryptor);
        return ResultSuccess();
    }

    Result AesCtrCounterExtendedStorage::Initialize(IAllocator *allocator, const void *key, size_t key_size, u32 secure_value, fs::SubStorage data_storage, fs::SubStorage table_storage) {
        /* Read and verify the bucket tree header. */
        BucketTree::Header header;
        R_TRY(table_storage.Read(0, std::addressof(header), sizeof(header)));
        R_TRY(header.Verify());

        /* Determine extents. */
        const auto node_storage_size    = QueryNodeStorageSize(header.entry_count);
        const auto entry_storage_size   = QueryEntryStorageSize(header.entry_count);
        const auto node_storage_offset  = QueryHeaderStorageSize();
        const auto entry_storage_offset = node_storage_offset + node_storage_size;

        /* Create a software decryptor. */
        std::unique_ptr<IDecryptor> sw_decryptor;
        R_TRY(CreateSoftwareDecryptor(std::addressof(sw_decryptor)));

        /* Initialize. */
        return this->Initialize(allocator, key, key_size, secure_value, 0, data_storage, fs::SubStorage(std::addressof(table_storage), node_storage_offset, node_storage_size), fs::SubStorage(std::addressof(table_storage), entry_storage_offset, entry_storage_size), header.entry_count, std::move(sw_decryptor));
    }

    Result AesCtrCounterExtendedStorage::Initialize(IAllocator *allocator, const void *key, size_t key_size, u32 secure_value, s64 counter_offset, fs::SubStorage data_storage, fs::SubStorage node_storage, fs::SubStorage entry_storage, s32 entry_count, std::unique_ptr<IDecryptor> &&decryptor) {
        /* Validate preconditions. */
        AMS_ASSERT(key != nullptr);
        AMS_ASSERT(key_size == KeySize);
        AMS_ASSERT(counter_offset >= 0);
        AMS_ASSERT(decryptor != nullptr);

        /* Initialize the bucket tree table. */
        R_TRY(this->table.Initialize(allocator, node_storage, entry_storage, NodeSize, sizeof(Entry), entry_count));

        /* Set members. */
        this->data_storage = data_storage;
        std::memcpy(this->key, key, key_size);
        this->secure_value = secure_value;
        this->counter_offset = counter_offset;
        this->decryptor = std::move(decryptor);

        return ResultSuccess();
    }

    void AesCtrCounterExtendedStorage::Finalize() {
        if (this->IsInitialized()) {
            this->table.Finalize();
            this->data_storage = fs::SubStorage();
        }
    }

    Result AesCtrCounterExtendedStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(this->IsInitialized());

        /* Allow zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr,                  fs::ResultNullptrArgument());
        R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidOffset());
        R_UNLESS(util::IsAligned(size, BlockSize),   fs::ResultInvalidSize());
        R_UNLESS(this->table.Includes(offset, size), fs::ResultOutOfRange());

        /* Read the data. */
        R_TRY(this->data_storage.Read(offset, buffer, size));

        /* Temporarily increase our thread priority. */
        ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

        /* Find the offset in our tree. */
        BucketTree::Visitor visitor;
        R_TRY(this->table.Find(std::addressof(visitor), offset));
        {
            const auto entry_offset = visitor.Get<Entry>()->GetOffset();
            R_UNLESS(util::IsAligned(entry_offset, BlockSize),                fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
            R_UNLESS(0 <= entry_offset && this->table.Includes(entry_offset), fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
        }

        /* Prepare to read in chunks. */
        u8 *cur_data = static_cast<u8 *>(buffer);
        auto cur_offset = offset;
        const auto end_offset = offset + static_cast<s64>(size);

        while (cur_offset < end_offset) {
            /* Get the current entry. */
            const auto cur_entry = *visitor.Get<Entry>();

            /* Get and validate the entry's offset. */
            const auto cur_entry_offset = cur_entry.GetOffset();
            R_UNLESS(cur_entry_offset <= cur_offset, fs::ResultInvalidAesCtrCounterExtendedEntryOffset());

            /* Get and validate the next entry offset. */
            s64 next_entry_offset;
            if (visitor.CanMoveNext()) {
                R_TRY(visitor.MoveNext());
                next_entry_offset = visitor.Get<Entry>()->GetOffset();
                R_UNLESS(this->table.Includes(next_entry_offset), fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
            } else {
                next_entry_offset = this->table.GetEnd();
            }
            R_UNLESS(util::IsAligned(next_entry_offset, BlockSize), fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
            R_UNLESS(cur_offset < next_entry_offset,                fs::ResultInvalidAesCtrCounterExtendedEntryOffset());

            /* Get the offset of the entry in the data we read. */
            const auto data_offset = cur_offset - cur_entry_offset;
            const auto data_size   = (next_entry_offset - cur_entry_offset) - data_offset;
            AMS_ASSERT(data_size > 0);

            /* Determine how much is left. */
            const auto remaining_size = end_offset - cur_offset;
            const auto cur_size       = static_cast<size_t>(std::min(remaining_size, data_size));
            AMS_ASSERT(cur_size <= size);

            /* Make the CTR for the data we're decrypting. */
            const auto counter_offset = this->counter_offset + cur_entry_offset + data_offset;
            NcaAesCtrUpperIv upper_iv = { .part = { .generation = static_cast<u32>(cur_entry.generation), .secure_value = this->secure_value } };

            u8 iv[IvSize];
            AesCtrStorage::MakeIv(iv, IvSize, upper_iv.value, counter_offset);

            /* Decrypt. */
            this->decryptor->Decrypt(cur_data, cur_size, this->key, KeySize, iv, IvSize);

            /* Advance. */
            cur_data   += cur_size;
            cur_offset += cur_size;
        }

        return ResultSuccess();
    }

    Result AesCtrCounterExtendedStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case fs::OperationId::InvalidateCache:
                {
                    /* Validate preconditions. */
                    AMS_ASSERT(offset >= 0);
                    AMS_ASSERT(this->IsInitialized());

                    /* Succeed if there's nothing to operate on. */
                    R_SUCCEED_IF(size == 0);

                    /* Validate arguments. */
                    R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidOffset());
                    R_UNLESS(util::IsAligned(size, BlockSize),   fs::ResultInvalidSize());
                    R_UNLESS(this->table.Includes(offset, size), fs::ResultOutOfRange());

                    /* Invalidate our table's cache. */
                    R_TRY(this->table.InvalidateCache());

                    /* Operate on our data storage. */
                    R_TRY(this->data_storage.OperateRange(dst, dst_size, op_id, offset, size, src, src_size));

                    return ResultSuccess();
                }
            case fs::OperationId::QueryRange:
                {
                    /* Validate preconditions. */
                    AMS_ASSERT(offset >= 0);
                    AMS_ASSERT(this->IsInitialized());

                    /* Validate that we have an output range info. */
                    R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                    R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                    /* Succeed if there's nothing to operate on. */
                    if (size == 0) {
                        reinterpret_cast<fs::QueryRangeInfo *>(dst)->Clear();
                        return ResultSuccess();
                    }

                    /* Validate arguments. */
                    R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidOffset());
                    R_UNLESS(util::IsAligned(size, BlockSize),   fs::ResultInvalidSize());
                    R_UNLESS(this->table.Includes(offset, size), fs::ResultOutOfRange());

                    /* Operate on our data storage. */
                    R_TRY(this->data_storage.OperateRange(dst, dst_size, op_id, offset, size, src, src_size));

                    /* Add in new flags. */
                    fs::QueryRangeInfo new_info;
                    new_info.Clear();
                    new_info.aes_ctr_key_type = static_cast<s32>(this->decryptor->HasExternalDecryptionKey() ? fs::AesCtrKeyTypeFlag::ExternalKeyForHardwareAes : fs::AesCtrKeyTypeFlag::InternalKeyForHardwareAes);

                    /*  Merge in the new info. */
                    reinterpret_cast<fs::QueryRangeInfo *>(dst)->Merge(new_info);

                    return ResultSuccess();
                }
            default:
                return fs::ResultUnsupportedOperationInAesCtrCounterExtendedStorageC();
        }
    }

    void SoftwareDecryptor::Decrypt(void *buf, size_t buf_size, const void *enc_key, size_t enc_key_size, void *iv, size_t iv_size) {
        crypto::DecryptAes128Ctr(buf, buf_size, enc_key, enc_key_size, iv, iv_size, buf, buf_size);
    }

    void ExternalDecryptor::Decrypt(void *buf, size_t buf_size, const void *enc_key, size_t enc_key_size, void *iv, size_t iv_size) {
        /* Validate preconditions. */
        AMS_ASSERT(buf != nullptr);
        AMS_ASSERT(enc_key != nullptr);
        AMS_ASSERT(enc_key_size == KeySize);
        AMS_ASSERT(iv != nullptr);
        AMS_ASSERT(iv_size == IvSize);

        /* Copy the ctr. */
        u8 ctr[IvSize];
        std::memcpy(ctr, iv, IvSize);

        /* Setup tracking. */
        size_t remaining_size = buf_size;
        s64 cur_offset = 0;

        /* Allocate a pooled buffer for decryption. */
        PooledBuffer pooled_buffer;
        pooled_buffer.AllocateParticularlyLarge(buf_size, BlockSize);
        AMS_ASSERT(pooled_buffer.GetSize() > 0 && util::IsAligned(pooled_buffer.GetSize(), BlockSize));

        /* Read and decrypt in chunks. */
        while (remaining_size > 0) {
            size_t cur_size = std::min(pooled_buffer.GetSize(), remaining_size);
            u8 *dst = static_cast<u8 *>(buf) + cur_offset;

            this->decrypt_function(pooled_buffer.GetBuffer(), cur_size, this->key_index, enc_key, enc_key_size, ctr, IvSize, dst, cur_size);

            std::memcpy(dst, pooled_buffer.GetBuffer(), cur_size);

            cur_offset     += cur_size;
            remaining_size -= cur_size;

            if (remaining_size > 0) {
                AddCounter(ctr, IvSize, cur_size / BlockSize);
            }
        }
    }

}
