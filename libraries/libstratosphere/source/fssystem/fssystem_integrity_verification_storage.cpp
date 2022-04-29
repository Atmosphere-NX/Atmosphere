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

    void IntegrityVerificationStorage::Initialize(fs::SubStorage hs, fs::SubStorage ds, s64 verif_block_size, s64 upper_layer_verif_block_size, fs::IBufferManager *bm, fssystem::IHash256GeneratorFactory *hgf, const util::optional<fs::HashSalt> &salt, bool is_real_data, bool is_writable, bool allow_cleared_blocks) {
        /* Validate preconditions. */
        AMS_ASSERT(verif_block_size >= HashSize);
        AMS_ASSERT(bm != nullptr);
        AMS_ASSERT(hgf != nullptr);

        /* Set storages. */
        m_hash_storage = hs;
        m_data_storage = ds;

        /* Set hash generator factory. */
        m_hash_generator_factory = hgf;

        /* Set verification block sizes. */
        m_verification_block_size  = verif_block_size;
        m_verification_block_order = ILog2(static_cast<u32>(verif_block_size));
        AMS_ASSERT(m_verification_block_size == (1l << m_verification_block_order));

        /* Set buffer manager. */
        m_buffer_manager = bm;

        /* Set upper layer block sizes. */
        upper_layer_verif_block_size           = std::max(upper_layer_verif_block_size, HashSize);
        m_upper_layer_verification_block_size  = upper_layer_verif_block_size;
        m_upper_layer_verification_block_order = ILog2(static_cast<u32>(upper_layer_verif_block_size));
        AMS_ASSERT(m_upper_layer_verification_block_size == (1l << m_upper_layer_verification_block_order));

        /* Validate sizes. */
        {
            s64 hash_size = 0;
            s64 data_size = 0;
            AMS_ASSERT(R_SUCCEEDED(m_hash_storage.GetSize(std::addressof(hash_size))));
            AMS_ASSERT(R_SUCCEEDED(m_data_storage.GetSize(std::addressof(data_size))));
            AMS_ASSERT(((hash_size / HashSize) * m_verification_block_size) >= data_size);
            AMS_UNUSED(hash_size, data_size);
        }

        /* Set salt. */
        m_salt = salt;

        /* Set data, writable, and allow cleared. */
        m_is_real_data         = is_real_data;
        m_is_writable          = is_writable;
        m_allow_cleared_blocks = allow_cleared_blocks;
    }

    void IntegrityVerificationStorage::Finalize() {
        if (m_buffer_manager != nullptr) {
            m_hash_storage = fs::SubStorage();
            m_data_storage = fs::SubStorage();
            m_buffer_manager = nullptr;
        }
    }

    Result IntegrityVerificationStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Although we support zero-size reads, we expect non-zero sizes. */
        AMS_ASSERT(size != 0);

        /* Validate other preconditions. */
        AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(m_verification_block_size)));
        AMS_ASSERT(util::IsAligned(size,   static_cast<size_t>(m_verification_block_size)));

        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Validate the offset. */
        s64 data_size;
        R_TRY(m_data_storage.GetSize(std::addressof(data_size)));
        R_UNLESS(offset <= data_size, fs::ResultInvalidOffset());

        /* Validate the access range. */
        R_TRY(IStorage::CheckAccessRange(offset, size, util::AlignUp(data_size, static_cast<size_t>(m_verification_block_size))));

        /* Determine the read extents. */
        size_t read_size = size;
        if (static_cast<s64>(offset + read_size) > data_size) {
            /* Determine the padding sizes. */
            s64 padding_offset  = data_size - offset;
            size_t padding_size = static_cast<size_t>(m_verification_block_size - (padding_offset & (m_verification_block_size - 1)));
            AMS_ASSERT(static_cast<s64>(padding_size) < m_verification_block_size);

            /* Clear the padding. */
            std::memset(static_cast<u8 *>(buffer) + padding_offset, 0, padding_size);

            /* Set the new in-bounds size. */
            read_size = static_cast<size_t>(data_size - offset);
        }

        /* Perform the read. */
        {
            auto clear_guard = SCOPE_GUARD { std::memset(buffer, 0, size); };
            R_TRY(m_data_storage.Read(offset, buffer, read_size));
            clear_guard.Cancel();
        }

        /* Verify the signatures. */
        Result verify_hash_result = ResultSuccess();

        /* Create hash generator. */
        std::unique_ptr<IHash256Generator> generator = nullptr;
        R_TRY(m_hash_generator_factory->Create(std::addressof(generator)));

        /* Prepare to validate the signatures. */
        const auto signature_count = size >> m_verification_block_order;
        PooledBuffer signature_buffer(signature_count * sizeof(BlockHash), sizeof(BlockHash));
        const auto buffer_count = std::min(signature_count, signature_buffer.GetSize() / sizeof(BlockHash));

        size_t verified_count = 0;
        while (verified_count < signature_count) {
            /* Read the current signatures. */
            const auto cur_count = std::min(buffer_count, signature_count - verified_count);
            auto cur_result = this->ReadBlockSignature(signature_buffer.GetBuffer(), signature_buffer.GetSize(), offset + (verified_count << m_verification_block_order), cur_count << m_verification_block_order);

            /* Temporarily increase our priority. */
            ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

            /* Loop over each signature we read. */
            for (size_t i = 0; i < cur_count && R_SUCCEEDED(cur_result); ++i) {
                const auto verified_size = (verified_count + i) << m_verification_block_order;
                u8 *cur_buf = static_cast<u8 *>(buffer) + verified_size;
                cur_result = this->VerifyHash(cur_buf, reinterpret_cast<BlockHash *>(signature_buffer.GetBuffer()) + i, generator);

                /* If the data is corrupted, clear the corrupted parts. */
                if (fs::ResultIntegrityVerificationStorageCorrupted::Includes(cur_result)) {
                    std::memset(cur_buf, 0, m_verification_block_size);

                    /* Set the result if we should. */
                    if (!fs::ResultClearedRealDataVerificationFailed::Includes(cur_result) && !m_allow_cleared_blocks) {
                        verify_hash_result = cur_result;
                    }

                    cur_result = ResultSuccess();
                }
            }

            /* If we failed, clear and return. */
            if (R_FAILED(cur_result)) {
                std::memset(buffer, 0, size);
                R_THROW(cur_result);
            }

            /* Advance. */
            verified_count += cur_count;
        }

        R_RETURN(verify_hash_result);
    }

    Result IntegrityVerificationStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr,                          fs::ResultNullptrArgument());

        /* Check the offset/size. */
        R_TRY(IStorage::CheckOffsetAndSize(offset, size));

        /* Validate the offset. */
        s64 data_size;
        R_TRY(m_data_storage.GetSize(std::addressof(data_size)));
        R_UNLESS(offset < data_size, fs::ResultInvalidOffset());

        /* Validate the access range. */
        R_TRY(IStorage::CheckAccessRange(offset, size, util::AlignUp(data_size, static_cast<size_t>(m_verification_block_size))));

        /* Validate preconditions. */
        AMS_ASSERT(util::IsAligned(offset, m_verification_block_size));
        AMS_ASSERT(util::IsAligned(size,   m_verification_block_size));
        AMS_ASSERT(offset <= data_size);
        AMS_ASSERT(static_cast<s64>(offset + size) < data_size + m_verification_block_size);

        /* Validate that if writing past the end, all extra data is zero padding. */
        if (static_cast<s64>(offset + size) > data_size) {
            const u8 *padding_cur = static_cast<const u8 *>(buffer) + data_size - offset;
            const u8 *padding_end = padding_cur + (offset + size - data_size);

            while (padding_cur < padding_end) {
                AMS_ASSERT((*padding_cur) == 0);
                ++padding_cur;
            }
        }

        /* Determine the unpadded size to write. */
        auto write_size = size;
        if (static_cast<s64>(offset + write_size) > data_size) {
            write_size = static_cast<size_t>(data_size - offset);
            R_SUCCEED_IF(write_size == 0);
        }

        /* Determine the size we're writing in blocks. */
        const auto aligned_write_size = util::AlignUp(write_size, m_verification_block_size);

        /* Write the updated block signatures. */
        Result update_result = ResultSuccess();
        size_t updated_count = 0;
        {
            const auto signature_count = aligned_write_size >> m_verification_block_order;
            PooledBuffer signature_buffer(signature_count * sizeof(BlockHash), sizeof(BlockHash));
            const auto buffer_count = std::min(signature_count, signature_buffer.GetSize() / sizeof(BlockHash));

            /* Create hash generator. */
            std::unique_ptr<IHash256Generator> generator = nullptr;
            R_TRY(m_hash_generator_factory->Create(std::addressof(generator)));

            while (updated_count < signature_count) {
                const auto cur_count = std::min(buffer_count, signature_count - updated_count);

                /* Calculate the hash with temporarily increased priority. */
                {
                    ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

                    for (size_t i = 0; i < cur_count; ++i) {
                        const auto updated_size = (updated_count + i) << m_verification_block_order;
                        this->CalcBlockHash(reinterpret_cast<BlockHash *>(signature_buffer.GetBuffer()) + i, reinterpret_cast<const u8 *>(buffer) + updated_size, generator);
                    }
                }

                /* Write the new block signatures. */
                if (R_FAILED((update_result = this->WriteBlockSignature(signature_buffer.GetBuffer(), signature_buffer.GetSize(), offset + (updated_count << m_verification_block_order), cur_count << m_verification_block_order)))) {
                    break;
                }

                /* Advance. */
                updated_count += cur_count;
            }
        }

        /* Write the data. */
        R_TRY(m_data_storage.Write(offset, buffer, std::min(write_size, updated_count << m_verification_block_order)));

        R_RETURN(update_result);
    }

    Result IntegrityVerificationStorage::GetSize(s64 *out) {
        R_RETURN(m_data_storage.GetSize(out));
    }

    Result IntegrityVerificationStorage::Flush() {
        /* Flush both storages. */
        R_TRY(m_hash_storage.Flush());
        R_TRY(m_data_storage.Flush());
        R_SUCCEED();
    }

    Result IntegrityVerificationStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        /* Validate preconditions. */
        if (op_id != fs::OperationId::Invalidate) {
            AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(m_verification_block_size)));
            AMS_ASSERT(util::IsAligned(size,   static_cast<size_t>(m_verification_block_size)));
        }

        switch (op_id) {
            case fs::OperationId::FillZero:
                {
                    /* FillZero should only be called for writable storages. */
                    AMS_ASSERT(m_is_writable);

                    /* Validate the range. */
                    s64 data_size = 0;
                    R_TRY(m_data_storage.GetSize(std::addressof(data_size)));
                    R_UNLESS(0 <= offset && offset <= data_size, fs::ResultInvalidOffset());

                    /* Determine the extents to clear. */
                    const auto sign_offset = (offset >> m_verification_block_order) * HashSize;
                    const auto sign_size   = (std::min(size, data_size - offset) >> m_verification_block_order) * HashSize;

                    /* Allocate a work buffer. */
                    const auto buf_size = static_cast<size_t>(std::min(sign_size, static_cast<s64>(1) << (m_upper_layer_verification_block_order + 2)));
                    std::unique_ptr<char[], fs::impl::Deleter> buf = fs::impl::MakeUnique<char[]>(buf_size);
                    R_UNLESS(buf != nullptr, fs::ResultAllocationMemoryFailedInIntegrityVerificationStorageA());

                    /* Clear the work buffer. */
                    std::memset(buf.get(), 0, buf_size);

                    /* Clear in chunks. */
                    auto remaining_size = sign_size;

                    while (remaining_size > 0) {
                        const auto cur_size = static_cast<size_t>(std::min(remaining_size, static_cast<s64>(buf_size)));
                        R_TRY(m_hash_storage.Write(sign_offset + sign_size - remaining_size, buf.get(), cur_size));
                        remaining_size -= cur_size;
                    }

                    R_SUCCEED();
                }
            case fs::OperationId::DestroySignature:
                {
                    /* DestroySignature should only be called for save data. */
                    AMS_ASSERT(m_is_writable);

                    /* Validate the range. */
                    s64 data_size = 0;
                    R_TRY(m_data_storage.GetSize(std::addressof(data_size)));
                    R_UNLESS(0 <= offset && offset <= data_size, fs::ResultInvalidOffset());

                    /* Determine the extents to clear the signature for. */
                    const auto sign_offset = (offset >> m_verification_block_order) * HashSize;
                    const auto sign_size   = (std::min(size, data_size - offset) >> m_verification_block_order) * HashSize;

                    /* Allocate a work buffer. */
                    std::unique_ptr<char[], fs::impl::Deleter> buf = fs::impl::MakeUnique<char[]>(sign_size);
                    R_UNLESS(buf != nullptr, fs::ResultAllocationMemoryFailedInIntegrityVerificationStorageB());

                    /* Read the existing signature. */
                    R_TRY(m_hash_storage.Read(sign_offset, buf.get(), sign_size));

                    /* Clear the signature. */
                    /* This flips all bits other than the verification bit. */
                    for (auto i = 0; i < sign_size; ++i) {
                        buf[i] ^= ((i + 1) % HashSize == 0 ? 0x7F : 0xFF);
                    }

                    /* Write the cleared signature. */
                    R_RETURN(m_hash_storage.Write(sign_offset, buf.get(), sign_size));
                }
            case fs::OperationId::Invalidate:
                {
                    /* Only allow cache invalidation read-only storages. */
                    R_UNLESS(!m_is_writable, fs::ResultUnsupportedOperateRangeForWritableIntegrityVerificationStorage());


                    /* Operate on our storages. */
                    R_TRY(m_hash_storage.OperateRange(op_id, 0, std::numeric_limits<s64>::max()));
                    R_TRY(m_data_storage.OperateRange(op_id, offset, size));

                    R_SUCCEED();
                }
            case fs::OperationId::QueryRange:
                {
                    /* Validate the range. */
                    s64 data_size = 0;
                    R_TRY(m_data_storage.GetSize(std::addressof(data_size)));
                    R_UNLESS(0 <= offset && offset <= data_size, fs::ResultInvalidOffset());

                    /* Determine the real size to query. */
                    const auto actual_size = std::min(size, data_size - offset);

                    /* Query the data storage. */
                    R_RETURN(m_data_storage.OperateRange(dst, dst_size, op_id, offset, actual_size, src, src_size));
                }
            default:
                R_THROW(fs::ResultUnsupportedOperateRangeForIntegrityVerificationStorage());
        }
    }

    void IntegrityVerificationStorage::CalcBlockHash(BlockHash *out, const void *buffer, size_t block_size, std::unique_ptr<fssystem::IHash256Generator> &generator) const {
        /* Hash procedure depends on whether or not we're writable. */
        if (m_is_writable) {
            /* Compute the hash with or without the hash salt, if we have one. */
            if (m_salt.has_value()) {
                /* Initialize the generator. */
                generator->Initialize();

                /* Hash the salt. */
                generator->Update(m_salt->value, sizeof(m_salt->value));

                /* Update with the buffer and get the hash. */
                generator->Update(buffer, block_size);
                generator->GetHash(out, sizeof(*out));
            } else {
                /* If we have no hash salt, just calculate the hash. */
                m_hash_generator_factory->GenerateHash(out, sizeof(*out), buffer, block_size);
            }

            /* Set the validation bit. */
            SetValidationBit(out);
        } else {
            /* If we're not writable, just calculate the hash. */
            m_hash_generator_factory->GenerateHash(out, sizeof(*out), buffer, block_size);
        }
    }

    Result IntegrityVerificationStorage::ReadBlockSignature(void *dst, size_t dst_size, s64 offset, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(m_verification_block_size)));
        AMS_ASSERT(util::IsAligned(size, static_cast<size_t>(m_verification_block_size)));

        /* Determine where to read the signature. */
        const s64 sign_offset = (offset >> m_verification_block_order) * HashSize;
        const auto sign_size  = static_cast<size_t>((size >> m_verification_block_order) * HashSize);
        AMS_ASSERT(dst_size >= sign_size);
        AMS_UNUSED(dst_size);

        /* Create a guard in the event of failure. */
        auto clear_guard = SCOPE_GUARD { std::memset(dst, 0, sign_size); };

        /* Validate that we can read the signature. */
        s64 hash_size;
        R_TRY(m_hash_storage.GetSize(std::addressof(hash_size)));
        const bool range_valid = static_cast<s64>(sign_offset + sign_size) <= hash_size;
        AMS_ASSERT(range_valid);
        R_UNLESS(range_valid, fs::ResultOutOfRange());

        /* Read the signature. */
        R_TRY(m_hash_storage.Read(sign_offset, dst, sign_size));

        /* We succeeded. */
        clear_guard.Cancel();
        R_SUCCEED();
    }

    Result IntegrityVerificationStorage::WriteBlockSignature(const void *src, size_t src_size, s64 offset, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(src != nullptr);
        AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(m_verification_block_size)));

        /* Determine where to write the signature. */
        const s64 sign_offset = (offset >> m_verification_block_order) * HashSize;
        const auto sign_size  = static_cast<size_t>((size >> m_verification_block_order) * HashSize);
        AMS_ASSERT(src_size >= sign_size);
        AMS_UNUSED(src_size);

        /* Write the signature. */
        R_TRY(m_hash_storage.Write(sign_offset, src, sign_size));

        /* We succeeded. */
        R_SUCCEED();
    }

    Result IntegrityVerificationStorage::VerifyHash(const void *buf, BlockHash *hash, std::unique_ptr<fssystem::IHash256Generator> &generator) {
        /* Validate preconditions. */
        AMS_ASSERT(buf != nullptr);
        AMS_ASSERT(hash != nullptr);

        /* Get the comparison hash. */
        auto &cmp_hash = *hash;

        /* If writable, check if the data is uninitialized. */
        if (m_is_writable) {
            bool is_cleared = false;
            R_TRY(this->IsCleared(std::addressof(is_cleared), cmp_hash));
            R_UNLESS(!is_cleared, fs::ResultClearedRealDataVerificationFailed());
        }

        /* Get the calculated hash. */
        BlockHash calc_hash;
        this->CalcBlockHash(std::addressof(calc_hash), buf, generator);

        /* Check that the signatures are equal. */
        if (!crypto::IsSameBytes(std::addressof(cmp_hash), std::addressof(calc_hash), sizeof(BlockHash))) {
            /* Clear the comparison hash. */
            std::memset(std::addressof(cmp_hash), 0, sizeof(cmp_hash));

            /* Return the appropriate result. */
            if (m_is_real_data) {
                R_THROW(fs::ResultUnclearedRealDataVerificationFailed());
            } else {
                R_THROW(fs::ResultNonRealDataVerificationFailed());
            }
        }

        R_SUCCEED();
    }

    Result IntegrityVerificationStorage::IsCleared(bool *is_cleared, const BlockHash &hash) {
        /* Validate preconditions. */
        AMS_ASSERT(is_cleared != nullptr);
        AMS_ASSERT(m_is_writable);

        /* Default to uncleared. */
        *is_cleared = false;

        /* Succeed if the validation bit is set. */
        R_SUCCEED_IF(IsValidationBit(std::addressof(hash)));

        /* Otherwise, we expect the hash to be all zero. */
        for (size_t i = 0; i < sizeof(hash.hash); ++i) {
            R_UNLESS(hash.hash[i] == 0, fs::ResultInvalidZeroHash());
        }

        /* Set cleared. */
        *is_cleared = true;
        R_SUCCEED();
    }

}
