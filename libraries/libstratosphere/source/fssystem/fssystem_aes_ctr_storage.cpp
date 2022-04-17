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

    template<fs::PointerToStorage BasePointer>
    void AesCtrStorage<BasePointer>::MakeIv(void *dst, size_t dst_size, u64 upper, s64 offset) {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size == IvSize);
        AMS_ASSERT(offset >= 0);
        AMS_UNUSED(dst_size);

        const uintptr_t out_addr = reinterpret_cast<uintptr_t>(dst);

        util::StoreBigEndian(reinterpret_cast<u64 *>(out_addr + 0),           upper);
        util::StoreBigEndian(reinterpret_cast<s64 *>(out_addr + sizeof(u64)), static_cast<s64>(offset / BlockSize));
    }

    template<fs::PointerToStorage BasePointer>
    AesCtrStorage<BasePointer>::AesCtrStorage(BasePointer base, const void *key, size_t key_size, const void *iv, size_t iv_size) : m_base_storage(std::move(base)) {
        AMS_ASSERT(m_base_storage != nullptr);
        AMS_ASSERT(key  != nullptr);
        AMS_ASSERT(iv   != nullptr);
        AMS_ASSERT(key_size == KeySize);
        AMS_ASSERT(iv_size  == IvSize);
        AMS_UNUSED(key_size, iv_size);

        std::memcpy(m_key, key, KeySize);
        std::memcpy(m_iv, iv, IvSize);
    }

    template<fs::PointerToStorage BasePointer>
    Result AesCtrStorage<BasePointer>::Read(s64 offset, void *buffer, size_t size) {
        /* Allow zero-size reads. */
        R_SUCCEED_IF(size == 0);

        /* Ensure buffer is valid. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* We can only read at block aligned offsets. */
        R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size, BlockSize),   fs::ResultInvalidArgument());

        /* Read the data. */
        R_TRY(m_base_storage->Read(offset, buffer, size));

        /* Prepare to decrypt the data, with temporarily increased priority. */
        ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

        /* Setup the counter. */
        char ctr[IvSize];
        std::memcpy(ctr, m_iv, IvSize);
        AddCounter(ctr, IvSize, offset / BlockSize);

        /* Decrypt, ensure we decrypt correctly. */
        auto dec_size = crypto::DecryptAes128Ctr(buffer, size, m_key, KeySize, ctr, IvSize, buffer, size);
        R_UNLESS(size == dec_size, fs::ResultUnexpectedInAesCtrStorageA());

        R_SUCCEED();
    }

    template<fs::PointerToStorage BasePointer>
    Result AesCtrStorage<BasePointer>::Write(s64 offset, const void *buffer, size_t size) {
        /* Allow zero-size writes. */
        R_SUCCEED_IF(size == 0);

        /* Ensure buffer is valid. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* We can only write at block aligned offsets. */
        R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size, BlockSize),   fs::ResultInvalidArgument());

        /* Get a pooled buffer. */
        PooledBuffer pooled_buffer;
        const bool use_work_buffer = !IsDeviceAddress(buffer);
        if (use_work_buffer) {
            pooled_buffer.Allocate(size, BlockSize);
        }

        /* Setup the counter. */
        char ctr[IvSize];
        std::memcpy(ctr, m_iv, IvSize);
        AddCounter(ctr, IvSize, offset / BlockSize);

        /* Loop until all data is written. */
        size_t remaining = size;
        s64 cur_offset = 0;
        while (remaining > 0) {
            /* Determine data we're writing and where. */
            const size_t write_size = use_work_buffer ? std::min(pooled_buffer.GetSize(), remaining) : remaining;
            void *write_buf         = use_work_buffer ? pooled_buffer.GetBuffer() : const_cast<void *>(buffer);

            /* Encrypt the data, with temporarily increased priority. */
            {
                ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

                auto enc_size = crypto::EncryptAes128Ctr(write_buf, write_size, m_key, KeySize, ctr, IvSize, reinterpret_cast<const char *>(buffer) + cur_offset, write_size);
                R_UNLESS(enc_size == write_size, fs::ResultUnexpectedInAesCtrStorageA());
            }

            /* Write the encrypted data. */
            R_TRY(m_base_storage->Write(offset + cur_offset, write_buf, write_size));

            /* Advance. */
            cur_offset += write_size;
            remaining  -= write_size;
            if (remaining > 0) {
                AddCounter(ctr, IvSize, write_size / BlockSize);
            }
        }

        R_SUCCEED();
    }

    template<fs::PointerToStorage BasePointer>
    Result AesCtrStorage<BasePointer>::Flush() {
        R_RETURN(m_base_storage->Flush());
    }

    template<fs::PointerToStorage BasePointer>
    Result AesCtrStorage<BasePointer>::SetSize(s64 size) {
        AMS_UNUSED(size);
        R_THROW(fs::ResultUnsupportedSetSizeForAesCtrStorage());
    }

    template<fs::PointerToStorage BasePointer>
    Result AesCtrStorage<BasePointer>::GetSize(s64 *out) {
        R_RETURN(m_base_storage->GetSize(out));
    }

    template<fs::PointerToStorage BasePointer>
    Result AesCtrStorage<BasePointer>::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        /* If operation isn't invalidate, special case. */
        if (op_id != fs::OperationId::Invalidate) {
            /* Handle the zero-size case. */
            if (size == 0) {
                if (op_id == fs::OperationId::QueryRange) {
                    R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                    R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                    reinterpret_cast<fs::QueryRangeInfo *>(dst)->Clear();
                }

                R_SUCCEED();
            }

            /* Ensure alignment. */
            R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidArgument());
            R_UNLESS(util::IsAligned(size, BlockSize),   fs::ResultInvalidArgument());
        }

        switch (op_id) {
            case fs::OperationId::QueryRange:
                {
                    R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                    R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                    R_TRY(m_base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));

                    fs::QueryRangeInfo info;
                    info.Clear();
                    info.aes_ctr_key_type = static_cast<s32>(fs::AesCtrKeyTypeFlag::InternalKeyForSoftwareAes);

                    reinterpret_cast<fs::QueryRangeInfo *>(dst)->Merge(info);
                }
                break;
            default:
                {
                    R_TRY(m_base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                }
                break;
        }

        R_SUCCEED();
    }

    template class AesCtrStorage<fs::IStorage *>;
    template class AesCtrStorage<std::shared_ptr<fs::IStorage>>;

}
