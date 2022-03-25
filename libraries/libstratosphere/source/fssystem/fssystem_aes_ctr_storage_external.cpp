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

    AesCtrStorageExternal::AesCtrStorageExternal(std::shared_ptr<fs::IStorage> bs, const void *enc_key, size_t enc_key_size, const void *iv, size_t iv_size, DecryptAesCtrFunction df, s32 kidx, s32 kgen) : m_base_storage(std::move(bs)), m_decrypt_function(df), m_key_index(kidx), m_key_generation(kgen) {
        AMS_ASSERT(m_base_storage != nullptr);
        AMS_ASSERT(enc_key_size == KeySize);
        AMS_ASSERT(iv != nullptr);
        AMS_ASSERT(iv_size == IvSize);
        AMS_UNUSED(iv_size);

        std::memcpy(m_iv, iv, IvSize);
        std::memcpy(m_encrypted_key, enc_key, enc_key_size);
    }

    Result AesCtrStorageExternal::Read(s64 offset, void *buffer, size_t size) {
        /* Allow zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        /* NOTE: For some reason, Nintendo uses InvalidArgument instead of InvalidOffset/InvalidSize here. */
        R_UNLESS(buffer != nullptr,                  fs::ResultNullptrArgument());
        R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size,   BlockSize), fs::ResultInvalidArgument());

        /* Read the data. */
        R_TRY(m_base_storage->Read(offset, buffer, size));

        /* Temporarily increase our thread priority. */
        ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

        /* Allocate a pooled buffer for decryption. */
        PooledBuffer pooled_buffer;
        pooled_buffer.AllocateParticularlyLarge(size, BlockSize);
        AMS_ASSERT(pooled_buffer.GetSize() >= BlockSize);

        /* Setup the counter. */
        u8 ctr[IvSize];
        std::memcpy(ctr, m_iv, IvSize);
        AddCounter(ctr, IvSize, offset / BlockSize);

        /* Setup tracking. */
        size_t remaining_size = size;
        s64 cur_offset = 0;

        while (remaining_size > 0) {
            /* Get the current size to process. */
            size_t cur_size = std::min(pooled_buffer.GetSize(), remaining_size);
            char *dst = static_cast<char *>(buffer) + cur_offset;

            /* Decrypt into the temporary buffer */
            m_decrypt_function(pooled_buffer.GetBuffer(), cur_size, m_key_index, m_key_generation, m_encrypted_key, KeySize, ctr, IvSize, dst, cur_size);

            /* Copy to the destination. */
            std::memcpy(dst, pooled_buffer.GetBuffer(), cur_size);

            /* Update tracking. */
            cur_offset     += cur_size;
            remaining_size -= cur_size;

            if (remaining_size > 0) {
                AddCounter(ctr, IvSize, cur_size / BlockSize);
            }
        }

        R_SUCCEED();
    }

    Result AesCtrStorageExternal::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case fs::OperationId::QueryRange:
                {
                    /* Validate that we have an output range info. */
                    R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                    R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                    /* Operate on our base storage. */
                    R_TRY(m_base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));

                    /* Add in new flags. */
                    fs::QueryRangeInfo new_info;
                    new_info.Clear();
                    new_info.aes_ctr_key_type = static_cast<s32>(m_key_index >= 0 ? fs::AesCtrKeyTypeFlag::InternalKeyForHardwareAes : fs::AesCtrKeyTypeFlag::ExternalKeyForHardwareAes);

                    /* Merge the new info in. */
                    reinterpret_cast<fs::QueryRangeInfo *>(dst)->Merge(new_info);
                    R_SUCCEED();
                }
            default:
                {
                    /* Operate on our base storage. */
                    R_RETURN(m_base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                }
        }
    }

    Result AesCtrStorageExternal::GetSize(s64 *out) {
        R_RETURN(m_base_storage->GetSize(out));
    }

    Result AesCtrStorageExternal::Flush() {
        R_SUCCEED();
    }

    Result AesCtrStorageExternal::Write(s64 offset, const void *buffer, size_t size) {
        AMS_UNUSED(offset, buffer, size);
        R_THROW(fs::ResultUnsupportedWriteForAesCtrStorageExternal());
    }

    Result AesCtrStorageExternal::SetSize(s64 size) {
        AMS_UNUSED(size);
        R_THROW(fs::ResultUnsupportedSetSizeForAesCtrStorageExternal());
    }

}
