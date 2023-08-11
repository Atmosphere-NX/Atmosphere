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
                AesCtrCounterExtendedStorage::DecryptFunction m_decrypt_function;
                s32 m_key_index;
                s32 m_key_generation;
            public:
                ExternalDecryptor(AesCtrCounterExtendedStorage::DecryptFunction df, s32 key_idx, s32 key_gen) : m_decrypt_function(df), m_key_index(key_idx), m_key_generation(key_gen) {
                    AMS_ASSERT(m_decrypt_function != nullptr);
                }
            public:
                virtual void Decrypt(void *buf, size_t buf_size, const void *enc_key, size_t enc_key_size, void *iv, size_t iv_size) override final;
                virtual bool HasExternalDecryptionKey() const override final { return m_key_index < 0; }
        };

    }

    Result AesCtrCounterExtendedStorage::CreateExternalDecryptor(std::unique_ptr<IDecryptor> *out, DecryptFunction func, s32 key_index, s32 key_generation) {
        std::unique_ptr<IDecryptor> decryptor = std::make_unique<ExternalDecryptor>(func, key_index, key_generation);
        R_UNLESS(decryptor != nullptr, fs::ResultAllocationMemoryFailedInAesCtrCounterExtendedStorageA());
        *out = std::move(decryptor);
        R_SUCCEED();
    }

    Result AesCtrCounterExtendedStorage::CreateSoftwareDecryptor(std::unique_ptr<IDecryptor> *out) {
        std::unique_ptr<IDecryptor> decryptor = std::make_unique<SoftwareDecryptor>();
        R_UNLESS(decryptor != nullptr, fs::ResultAllocationMemoryFailedInAesCtrCounterExtendedStorageA());
        *out = std::move(decryptor);
        R_SUCCEED();
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
        R_RETURN(this->Initialize(allocator, key, key_size, secure_value, 0, data_storage, fs::SubStorage(std::addressof(table_storage), node_storage_offset, node_storage_size), fs::SubStorage(std::addressof(table_storage), entry_storage_offset, entry_storage_size), header.entry_count, std::move(sw_decryptor)));
    }

    Result AesCtrCounterExtendedStorage::Initialize(IAllocator *allocator, const void *key, size_t key_size, u32 secure_value, s64 counter_offset, fs::SubStorage data_storage, fs::SubStorage node_storage, fs::SubStorage entry_storage, s32 entry_count, std::unique_ptr<IDecryptor> &&decryptor) {
        /* Validate preconditions. */
        AMS_ASSERT(key != nullptr);
        AMS_ASSERT(key_size == KeySize);
        AMS_ASSERT(counter_offset >= 0);
        AMS_ASSERT(decryptor != nullptr);

        /* Initialize the bucket tree table. */
        if (entry_count > 0) {
            R_TRY(m_table.Initialize(allocator, node_storage, entry_storage, NodeSize, sizeof(Entry), entry_count));
        } else {
            m_table.Initialize(NodeSize, 0);
        }

        /* Set members. */
        m_data_storage   = data_storage;
        std::memcpy(m_key, key, key_size);
        m_secure_value   = secure_value;
        m_counter_offset = counter_offset;
        m_decryptor      = std::move(decryptor);

        R_SUCCEED();
    }

    void AesCtrCounterExtendedStorage::Finalize() {
        if (this->IsInitialized()) {
            m_table.Finalize();
            m_data_storage = fs::SubStorage();
        }
    }

    Result AesCtrCounterExtendedStorage::GetEntryList(Entry *out_entries, s32 *out_entry_count, s32 entry_count, s64 offset, s64 size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(size >= 0);
        AMS_ASSERT(this->IsInitialized());

        /* Clear the out count. */
        R_UNLESS(out_entry_count != nullptr, fs::ResultNullptrArgument());
        *out_entry_count = 0;

        /* Succeed if there's no range. */
        R_SUCCEED_IF(size == 0);

        /* If we have an output array, we need it to be non-null. */
        R_UNLESS(out_entries != nullptr || entry_count == 0, fs::ResultNullptrArgument());

        /* Check that our range is valid. */
        BucketTree::Offsets table_offsets;
        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

        R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

        /* Find the offset in our tree. */
        BucketTree::Visitor visitor;
        R_TRY(m_table.Find(std::addressof(visitor), offset));
        {
            const auto entry_offset = visitor.Get<Entry>()->GetOffset();
            R_UNLESS(0 <= entry_offset && table_offsets.IsInclude(entry_offset), fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
        }

        /* Prepare to loop over entries. */
        const auto end_offset = offset + static_cast<s64>(size);
        s32 count = 0;

        auto cur_entry = *visitor.Get<Entry>();
        while (cur_entry.GetOffset() < end_offset) {
            /* Try to write the entry to the out list. */
            if (entry_count != 0) {
                if (count >= entry_count) {
                    break;
                }
                std::memcpy(out_entries + count, std::addressof(cur_entry), sizeof(Entry));
            }

            count++;

            /* Advance. */
            if (visitor.CanMoveNext()) {
                R_TRY(visitor.MoveNext());
                cur_entry = *visitor.Get<Entry>();
            } else {
                break;
            }
        }

        /* Write the output count. */
        *out_entry_count = count;
        R_SUCCEED();
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

        BucketTree::Offsets table_offsets;
        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

        R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

        /* Read the data. */
        R_TRY(m_data_storage.Read(offset, buffer, size));

        /* Temporarily increase our thread priority. */
        ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

        /* Find the offset in our tree. */
        BucketTree::Visitor visitor;
        R_TRY(m_table.Find(std::addressof(visitor), offset));
        {
            const auto entry_offset = visitor.Get<Entry>()->GetOffset();
            R_UNLESS(util::IsAligned(entry_offset, BlockSize),                   fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
            R_UNLESS(0 <= entry_offset && table_offsets.IsInclude(entry_offset), fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
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
                R_UNLESS(table_offsets.IsInclude(next_entry_offset), fs::ResultInvalidAesCtrCounterExtendedEntryOffset());
            } else {
                next_entry_offset = table_offsets.end_offset;
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

            /* If necessary, perform decryption. */
            if (cur_entry.encryption_value == Entry::Encryption::Encrypted) {
                /* Make the CTR for the data we're decrypting. */
                const auto counter_offset = m_counter_offset + cur_entry_offset + data_offset;
                NcaAesCtrUpperIv upper_iv = { .part = { .generation = static_cast<u32>(cur_entry.generation), .secure_value = m_secure_value } };

                u8 iv[IvSize];
                AesCtrStorageByPointer::MakeIv(iv, IvSize, upper_iv.value, counter_offset);

                /* Decrypt. */
                m_decryptor->Decrypt(cur_data, cur_size, m_key, KeySize, iv, IvSize);
            }

            /* Advance. */
            cur_data   += cur_size;
            cur_offset += cur_size;
        }

        R_SUCCEED();
    }

    Result AesCtrCounterExtendedStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case fs::OperationId::Invalidate:
                {
                    /* Validate preconditions. */
                    AMS_ASSERT(this->IsInitialized());

                    /* Invalidate our table's cache. */
                    R_TRY(m_table.InvalidateCache());

                    /* Operate on our data storage. */
                    R_TRY(m_data_storage.OperateRange(fs::OperationId::Invalidate, 0, std::numeric_limits<s64>::max()));

                    R_SUCCEED();
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
                        R_SUCCEED();
                    }

                    /* Validate arguments. */
                    R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidOffset());
                    R_UNLESS(util::IsAligned(size, BlockSize),   fs::ResultInvalidSize());

                    BucketTree::Offsets table_offsets;
                    R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

                    R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

                    /* Operate on our data storage. */
                    R_TRY(m_data_storage.OperateRange(dst, dst_size, op_id, offset, size, src, src_size));

                    /* Add in new flags. */
                    fs::QueryRangeInfo new_info;
                    new_info.Clear();
                    new_info.aes_ctr_key_type = static_cast<s32>(m_decryptor->HasExternalDecryptionKey() ? fs::AesCtrKeyTypeFlag::ExternalKeyForHardwareAes : fs::AesCtrKeyTypeFlag::InternalKeyForHardwareAes);

                    /*  Merge in the new info. */
                    reinterpret_cast<fs::QueryRangeInfo *>(dst)->Merge(new_info);

                    R_SUCCEED();
                }
            default:
                R_THROW(fs::ResultUnsupportedOperateRangeForAesCtrCounterExtendedStorage());
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
        AMS_UNUSED(iv_size);

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

            m_decrypt_function(pooled_buffer.GetBuffer(), cur_size, m_key_index, m_key_generation, enc_key, enc_key_size, ctr, IvSize, dst, cur_size);

            std::memcpy(dst, pooled_buffer.GetBuffer(), cur_size);

            cur_offset     += cur_size;
            remaining_size -= cur_size;

            if (remaining_size > 0) {
                AddCounter(ctr, IvSize, cur_size / BlockSize);
            }
        }
    }

}
