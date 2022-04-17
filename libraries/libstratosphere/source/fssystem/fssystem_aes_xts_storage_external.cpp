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
    AesXtsStorageExternal<BasePointer>::AesXtsStorageExternal(BasePointer bs, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, size_t block_size, CryptAesXtsFunction ef, CryptAesXtsFunction df) : m_base_storage(std::move(bs)), m_block_size(block_size), m_encrypt_function(ef), m_decrypt_function(df) {
        AMS_ASSERT(key_size == KeySize);
        AMS_ASSERT(iv_size == IvSize);
        AMS_UNUSED(key_size, iv_size);

        if (key1 != nullptr) {
            std::memcpy(m_key[0], key1, KeySize);
        }

        if (key2 != nullptr) {
            std::memcpy(m_key[1], key2, KeySize);
        }

        std::memcpy(m_iv, iv, IvSize);
    }

    template<fs::PointerToStorage BasePointer>
    Result AesXtsStorageExternal<BasePointer>::Read(s64 offset, void *buffer, size_t size) {
        /* Allow zero size. */
        R_SUCCEED_IF(size == 0);

        /* Ensure buffer is valid. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure we can decrypt. */
        R_UNLESS(m_decrypt_function != nullptr, fs::ResultNullptrArgument());

        /* We can only read at block aligned offsets. */
        R_UNLESS(util::IsAligned(offset, AesBlockSize), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size,   AesBlockSize), fs::ResultInvalidArgument());

        /* Read the data. */
        R_TRY(m_base_storage->Read(offset, buffer, size));

        /* Temporarily increase our thread priority. */
        ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

        /* Setup the counter. */
        char ctr[IvSize];
        std::memcpy(ctr, m_iv, IvSize);
        AddCounter(ctr, IvSize, offset / m_block_size);

        /* Handle any unaligned data before the start. */
        size_t processed_size = 0;
        if ((offset % m_block_size) != 0) {
            /* Determine the size of the pre-data read. */
            const size_t skip_size = static_cast<size_t>(offset - util::AlignDown(offset, m_block_size));
            const size_t data_size = std::min(size, m_block_size - skip_size);

            /* Decrypt into a pooled buffer. */
            {
                PooledBuffer tmp_buf(m_block_size, m_block_size);
                AMS_ASSERT(tmp_buf.GetSize() >= m_block_size);

                std::memset(tmp_buf.GetBuffer(), 0, skip_size);
                std::memcpy(tmp_buf.GetBuffer() + skip_size, buffer, data_size);

                /* Decrypt. */
                R_TRY(m_decrypt_function(tmp_buf.GetBuffer(), m_block_size, m_key[0], m_key[1], KeySize, ctr, IvSize, tmp_buf.GetBuffer(), m_block_size));

                std::memcpy(buffer, tmp_buf.GetBuffer() + skip_size, data_size);
            }

            AddCounter(ctr, IvSize, 1);
            processed_size += data_size;
            AMS_ASSERT(processed_size == std::min(size, m_block_size - skip_size));
        }

        /* Decrypt aligned chunks. */
        char *cur = static_cast<char *>(buffer) + processed_size;
        size_t remaining = size - processed_size;
        while (remaining > 0) {
            const size_t cur_size = std::min(m_block_size, remaining);
            R_TRY(m_decrypt_function(cur, cur_size, m_key[0], m_key[1], KeySize, ctr, IvSize, cur, cur_size));

            remaining -= cur_size;
            cur       += cur_size;

            AddCounter(ctr, IvSize, 1);
        }

        R_SUCCEED();
    }

    template<fs::PointerToStorage BasePointer>
    Result AesXtsStorageExternal<BasePointer>::Write(s64 offset, const void *buffer, size_t size) {
        /* Allow zero-size writes. */
        R_SUCCEED_IF(size == 0);

        /* Ensure buffer is valid. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure we can encrypt. */
        R_UNLESS(m_encrypt_function != nullptr, fs::ResultNullptrArgument());

        /* We can only write at block aligned offsets. */
        R_UNLESS(util::IsAligned(offset, AesBlockSize), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size,   AesBlockSize), fs::ResultInvalidArgument());

        /* Get a pooled buffer. */
        PooledBuffer pooled_buffer;
        const bool use_work_buffer = !IsDeviceAddress(buffer);
        if (use_work_buffer) {
            pooled_buffer.Allocate(size, m_block_size);
        }

        /* Setup the counter. */
        char ctr[IvSize];
        std::memcpy(ctr, m_iv, IvSize);
        AddCounter(ctr, IvSize, offset / m_block_size);

        /* Handle any unaligned data before the start. */
        size_t processed_size = 0;
        if ((offset % m_block_size) != 0) {
            /* Determine the size of the pre-data read. */
            const size_t skip_size = static_cast<size_t>(offset - util::AlignDown(offset, m_block_size));
            const size_t data_size = std::min(size, m_block_size - skip_size);

            /* Encrypt into a pooled buffer. */
            {
                /* NOTE: Nintendo allocates a second pooled buffer here despite having one already allocated above. */
                PooledBuffer tmp_buf(m_block_size, m_block_size);
                AMS_ASSERT(tmp_buf.GetSize() >= m_block_size);

                std::memset(tmp_buf.GetBuffer(), 0, skip_size);
                std::memcpy(tmp_buf.GetBuffer() + skip_size, buffer, data_size);

                R_TRY(m_encrypt_function(tmp_buf.GetBuffer(), m_block_size, m_key[0], m_key[1], KeySize, ctr, IvSize, tmp_buf.GetBuffer(), m_block_size));

                R_TRY(m_base_storage->Write(offset, tmp_buf.GetBuffer() + skip_size, data_size));
            }

            AddCounter(ctr, IvSize, 1);
            processed_size += data_size;
            AMS_ASSERT(processed_size == std::min(size, m_block_size - skip_size));
        }

        /* Encrypt aligned chunks. */
        size_t remaining = size - processed_size;
        s64 cur_offset   = offset + processed_size;
        while (remaining > 0) {
            /* Determine data we're writing and where. */
            const size_t write_size = use_work_buffer ? std::min(pooled_buffer.GetSize(), remaining) : remaining;

            /* Encrypt the data, with temporarily increased priority. */
            {
                ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

                size_t remaining_write = write_size;
                size_t encrypt_offset  = 0;
                while (remaining_write > 0) {
                    const size_t cur_size = std::min(remaining_write, m_block_size);
                    const void *src = static_cast<const char *>(buffer) + processed_size + encrypt_offset;
                    void *dst = use_work_buffer ? pooled_buffer.GetBuffer() + encrypt_offset : const_cast<void *>(src);

                    R_TRY(m_encrypt_function(dst, cur_size, m_key[0], m_key[1], KeySize, ctr, IvSize, src, cur_size));

                    AddCounter(ctr, IvSize, 1);

                    encrypt_offset  += cur_size;
                    remaining_write -= cur_size;
                }
            }

            /* Write the encrypted data. */
            const void *write_buf = use_work_buffer ? pooled_buffer.GetBuffer() : static_cast<const char *>(buffer) + processed_size;
            R_TRY(m_base_storage->Write(cur_offset, write_buf, write_size));

            /* Advance. */
            cur_offset     += write_size;
            processed_size += write_size;
            remaining      -= write_size;
        }

        R_SUCCEED();
    }

    template<fs::PointerToStorage BasePointer>
    Result AesXtsStorageExternal<BasePointer>::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        /* Unless invalidating cache, check the arguments. */
        if (op_id != fs::OperationId::Invalidate) {
            /* Handle the zero size case. */
            R_SUCCEED_IF(size == 0);

            /* Ensure alignment. */
            R_UNLESS(util::IsAligned(offset, AesBlockSize), fs::ResultInvalidArgument());
            R_UNLESS(util::IsAligned(size, AesBlockSize),   fs::ResultInvalidArgument());
        }

        R_RETURN(m_base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
    }

    template<fs::PointerToStorage BasePointer>
    Result AesXtsStorageExternal<BasePointer>::GetSize(s64 *out) {
        R_RETURN(m_base_storage->GetSize(out));
    }

    template<fs::PointerToStorage BasePointer>
    Result AesXtsStorageExternal<BasePointer>::Flush() {
        R_RETURN(m_base_storage->Flush());
    }

    template<fs::PointerToStorage BasePointer>
    Result AesXtsStorageExternal<BasePointer>::SetSize(s64 size) {
        R_UNLESS(util::IsAligned(size, AesBlockSize), fs::ResultUnexpectedInAesXtsStorageA());

        R_RETURN(m_base_storage->SetSize(size));
    }

    template class AesXtsStorageExternal<fs::IStorage *>;
    template class AesXtsStorageExternal<std::shared_ptr<fs::IStorage>>;

}
