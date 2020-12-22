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

namespace ams::fssystem::save {

    Result IntegrityVerificationStorage::Initialize(fs::SubStorage hs, fs::SubStorage ds, s64 verif_block_size, s64 upper_layer_verif_block_size, IBufferManager *bm, const fs::HashSalt &salt, bool is_real_data, fs::StorageType storage_type) {
        /* Validate preconditions. */
        AMS_ASSERT(verif_block_size >= HashSize);
        AMS_ASSERT(bm != nullptr);

        /* Set storages. */
        this->hash_storage = hs;
        this->data_storage = ds;

        /* Set verification block sizes. */
        this->verification_block_size  = verif_block_size;
        this->verification_block_order = ILog2(static_cast<u32>(verif_block_size));
        AMS_ASSERT(this->verification_block_size == (1l << this->verification_block_order));

        /* Set buffer manager. */
        this->buffer_manager = bm;

        /* Set upper layer block sizes. */
        upper_layer_verif_block_size               = std::max(upper_layer_verif_block_size, HashSize);
        this->upper_layer_verification_block_size  = upper_layer_verif_block_size;
        this->upper_layer_verification_block_order = ILog2(static_cast<u32>(upper_layer_verif_block_size));
        AMS_ASSERT(this->upper_layer_verification_block_size == (1l << this->upper_layer_verification_block_order));

        /* Validate sizes. */
        {
            s64 hash_size = 0;
            s64 data_size = 0;
            AMS_ASSERT(R_SUCCEEDED(hash_storage.GetSize(std::addressof(hash_size))));
            AMS_ASSERT(R_SUCCEEDED(data_storage.GetSize(std::addressof(hash_size))));
            AMS_ASSERT(((hash_size / HashSize) * this->verification_block_size) >= data_size);
        }

        /* Set salt. */
        std::memcpy(this->salt.value, salt.value, fs::HashSalt::Size);

        /* Set data and storage type. */
        this->is_real_data = is_real_data;
        this->storage_type = storage_type;
        return ResultSuccess();
    }

    void IntegrityVerificationStorage::Finalize() {
        if (this->buffer_manager != nullptr) {
            this->hash_storage = fs::SubStorage();
            this->data_storage = fs::SubStorage();
            this->buffer_manager = nullptr;
        }
    }

    Result IntegrityVerificationStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Although we support zero-size reads, we expect non-zero sizes. */
        AMS_ASSERT(size != 0);

        /* Validate other preconditions. */
        AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(this->verification_block_size)));
        AMS_ASSERT(util::IsAligned(size,   static_cast<size_t>(this->verification_block_size)));

        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Validate the offset. */
        s64 data_size;
        R_TRY(this->data_storage.GetSize(std::addressof(data_size)));
        R_UNLESS(offset <= data_size, fs::ResultInvalidOffset());

        /* Validate the access range. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, util::AlignUp(data_size, static_cast<size_t>(this->verification_block_size))), fs::ResultOutOfRange());

        /* Determine the read extents. */
        size_t read_size = size;
        if (static_cast<s64>(offset + read_size) > data_size) {
            /* Determine the padding sizes. */
            s64 padding_offset  = data_size - offset;
            size_t padding_size = static_cast<size_t>(this->verification_block_size - (padding_offset & (this->verification_block_size - 1)));
            AMS_ASSERT(static_cast<s64>(padding_size) < this->verification_block_size);

            /* Clear the padding. */
            std::memset(static_cast<u8 *>(buffer) + padding_offset, 0, padding_size);

            /* Set the new in-bounds size. */
            read_size = static_cast<size_t>(data_size - offset);
        }

        /* Perform the read. */
        {
            auto clear_guard = SCOPE_GUARD { std::memset(buffer, 0, size); };
            R_TRY(this->data_storage.Read(offset, buffer, read_size));
            clear_guard.Cancel();
        }

        /* Prepare to validate the signatures. */
        const auto signature_count = size >> this->verification_block_order;
        PooledBuffer signature_buffer(signature_count * sizeof(BlockHash), sizeof(BlockHash));
        const auto buffer_count = std::min(signature_count, signature_buffer.GetSize() / sizeof(BlockHash));

        /* Verify the signatures. */
        Result verify_hash_result = ResultSuccess();

        size_t verified_count = 0;
        while (verified_count < signature_count) {
            /* Read the current signatures. */
            const auto cur_count = std::min(buffer_count, signature_count - verified_count);
            auto cur_result = this->ReadBlockSignature(signature_buffer.GetBuffer(), signature_buffer.GetSize(), offset + (verified_count << this->verification_block_order), cur_count << this->verification_block_order);

            /* Temporarily increase our priority. */
            ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

            /* Loop over each signature we read. */
            for (size_t i = 0; i < cur_count && R_SUCCEEDED(cur_result); ++i) {
                const auto verified_size = (verified_count + i) << this->verification_block_order;
                u8 *cur_buf = static_cast<u8 *>(buffer) + verified_size;
                cur_result = this->VerifyHash(cur_buf, reinterpret_cast<BlockHash *>(signature_buffer.GetBuffer()) + i);

                /* If the data is corrupted, clear the corrupted parts. */
                if (fs::ResultIntegrityVerificationStorageCorrupted::Includes(cur_result)) {
                    std::memset(cur_buf, 0, this->verification_block_size);

                    /* Set the result if we should. */
                    if (!fs::ResultClearedRealDataVerificationFailed::Includes(cur_result) && this->storage_type != fs::StorageType_Authoring) {
                        verify_hash_result = cur_result;
                    }

                    cur_result = ResultSuccess();
                }
            }

            /* If we failed, clear and return. */
            if (R_FAILED(cur_result)) {
                std::memset(buffer, 0, size);
                return cur_result;
            }

            /* Advance. */
            verified_count += cur_count;
        }

        return verify_hash_result;
    }

    Result IntegrityVerificationStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr,                          fs::ResultNullptrArgument());
        R_UNLESS(IStorage::CheckOffsetAndSize(offset, size), fs::ResultInvalidOffset());

        /* Validate the offset. */
        s64 data_size;
        R_TRY(this->data_storage.GetSize(std::addressof(data_size)));
        R_UNLESS(offset < data_size, fs::ResultInvalidOffset());

        /* Validate the access range. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, util::AlignUp(data_size, static_cast<size_t>(this->verification_block_size))), fs::ResultOutOfRange());

        /* Validate preconditions. */
        AMS_ASSERT(util::IsAligned(offset, this->verification_block_size));
        AMS_ASSERT(util::IsAligned(size,   this->verification_block_size));
        AMS_ASSERT(offset <= data_size);
        AMS_ASSERT(static_cast<s64>(offset + size) < data_size + this->verification_block_size);

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
        const auto aligned_write_size = util::AlignUp(write_size, this->verification_block_size);

        /* Write the updated block signatures. */
        Result update_result = ResultSuccess();
        size_t updated_count = 0;
        {
            const auto signature_count = aligned_write_size >> this->verification_block_order;
            PooledBuffer signature_buffer(signature_count * sizeof(BlockHash), sizeof(BlockHash));
            const auto buffer_count = std::min(signature_count, signature_buffer.GetSize() / sizeof(BlockHash));

            while (updated_count < signature_count) {
                const auto cur_count = std::min(buffer_count, signature_count - updated_count);

                /* Calculate the hash with temporarily increased priority. */
                {
                    ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

                    for (size_t i = 0; i < cur_count; ++i) {
                        const auto updated_size = (updated_count + i) << this->verification_block_order;
                        this->CalcBlockHash(reinterpret_cast<BlockHash *>(signature_buffer.GetBuffer()) + i, reinterpret_cast<const u8 *>(buffer) + updated_size);
                    }
                }

                /* Write the new block signatures. */
                if (R_FAILED((update_result = this->WriteBlockSignature(signature_buffer.GetBuffer(), signature_buffer.GetSize(), offset + (updated_count << this->verification_block_order), cur_count << this->verification_block_order)))) {
                    break;
                }

                /* Advance. */
                updated_count += cur_count;
            }
        }

        /* Write the data. */
        R_TRY(this->data_storage.Write(offset, buffer, std::min(write_size, updated_count << this->verification_block_order)));

        return update_result;
    }

    Result IntegrityVerificationStorage::GetSize(s64 *out) {
        return this->data_storage.GetSize(out);
    }

    Result IntegrityVerificationStorage::Flush() {
        /* Flush both storages. */
        R_TRY(this->hash_storage.Flush());
        R_TRY(this->data_storage.Flush());
        return ResultSuccess();
    }

    Result IntegrityVerificationStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        /* Validate preconditions. */
        AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(this->verification_block_size)));
        AMS_ASSERT(util::IsAligned(size,   static_cast<size_t>(this->verification_block_size)));

        switch (op_id) {
            case fs::OperationId::Clear:
                {
                    /* Clear should only be called for save data. */
                    AMS_ASSERT(this->storage_type == fs::StorageType_SaveData);

                    /* Validate the range. */
                    s64 data_size = 0;
                    R_TRY(this->data_storage.GetSize(std::addressof(data_size)));
                    R_UNLESS(0 <= offset && offset <= data_size, fs::ResultInvalidOffset());

                    /* Determine the extents to clear. */
                    const auto sign_offset = (offset >> this->verification_block_order) * HashSize;
                    const auto sign_size   = (std::min(size, data_size - offset) >> this->verification_block_order) * HashSize;

                    /* Allocate a work buffer. */
                    const auto buf_size = static_cast<size_t>(std::min(sign_size, static_cast<s64>(1) << (this->upper_layer_verification_block_order + 2)));
                    std::unique_ptr<char[], fs::impl::Deleter> buf = fs::impl::MakeUnique<char[]>(buf_size);
                    R_UNLESS(buf != nullptr, fs::ResultAllocationFailureInIntegrityVerificationStorageA());

                    /* Clear the work buffer. */
                    std::memset(buf.get(), 0, buf_size);

                    /* Clear in chunks. */
                    auto remaining_size = sign_size;

                    while (remaining_size > 0) {
                        const auto cur_size = static_cast<size_t>(std::min(remaining_size, static_cast<s64>(buf_size)));
                        R_TRY(this->hash_storage.Write(sign_offset + sign_size - remaining_size, buf.get(), cur_size));
                        remaining_size -= cur_size;
                    }

                    return ResultSuccess();
                }
            case fs::OperationId::ClearSignature:
                {
                    /* Clear Signature should only be called for save data. */
                    AMS_ASSERT(this->storage_type == fs::StorageType_SaveData);

                    /* Validate the range. */
                    s64 data_size = 0;
                    R_TRY(this->data_storage.GetSize(std::addressof(data_size)));
                    R_UNLESS(0 <= offset && offset <= data_size, fs::ResultInvalidOffset());

                    /* Determine the extents to clear the signature for. */
                    const auto sign_offset = (offset >> this->verification_block_order) * HashSize;
                    const auto sign_size   = (std::min(size, data_size - offset) >> this->verification_block_order) * HashSize;

                    /* Allocate a work buffer. */
                    std::unique_ptr<char[], fs::impl::Deleter> buf = fs::impl::MakeUnique<char[]>(sign_size);
                    R_UNLESS(buf != nullptr, fs::ResultAllocationFailureInIntegrityVerificationStorageB());

                    /* Read the existing signature. */
                    R_TRY(this->hash_storage.Read(sign_offset, buf.get(), sign_size));

                    /* Clear the signature. */
                    /* This sets all bytes to FF, with the verification bit cleared. */
                    for (auto i = 0; i < sign_size; ++i) {
                        buf[i] ^= ((i + 1) % HashSize == 0 ? 0x7F : 0xFF);
                    }

                    /* Write the cleared signature. */
                    return this->hash_storage.Write(sign_offset, buf.get(), sign_size);
                }
            case fs::OperationId::InvalidateCache:
                {
                    /* Only allow cache invalidation for RomFs. */
                    R_UNLESS(this->storage_type != fs::StorageType_SaveData, fs::ResultUnsupportedOperationInIntegrityVerificationStorageB());

                    /* Validate the range. */
                    s64 data_size = 0;
                    R_TRY(this->data_storage.GetSize(std::addressof(data_size)));
                    R_UNLESS(0 <= offset && offset <= data_size, fs::ResultInvalidOffset());

                    /* Determine the extents to invalidate. */
                    const auto sign_offset = (offset >> this->verification_block_order) * HashSize;
                    const auto sign_size   = (std::min(size, data_size - offset) >> this->verification_block_order) * HashSize;

                    /* Operate on our storages. */
                    R_TRY(this->hash_storage.OperateRange(dst, dst_size, op_id, sign_offset, sign_size, src, src_size));
                    R_TRY(this->data_storage.OperateRange(dst, dst_size, op_id, sign_offset, sign_size, src, src_size));

                    return ResultSuccess();
                }
            case fs::OperationId::QueryRange:
                {
                    /* Validate the range. */
                    s64 data_size = 0;
                    R_TRY(this->data_storage.GetSize(std::addressof(data_size)));
                    R_UNLESS(0 <= offset && offset <= data_size, fs::ResultInvalidOffset());

                    /* Determine the real size to query. */
                    const auto actual_size = std::min(size, data_size - offset);

                    /* Query the data storage. */
                    R_TRY(this->data_storage.OperateRange(dst, dst_size, op_id, offset, actual_size, src, src_size));

                    return ResultSuccess();
                }
            default:
                return fs::ResultUnsupportedOperationInIntegrityVerificationStorageC();
        }
    }

    void IntegrityVerificationStorage::CalcBlockHash(BlockHash *out, const void *buffer, size_t block_size) const {
        /* Create a sha256 generator. */
        crypto::Sha256Generator sha;
        sha.Initialize();

        /* If calculating for save data, hash the salt. */
        if (this->storage_type == fs::StorageType_SaveData) {
            sha.Update(this->salt.value, sizeof(this->salt));
        }

        /* Update with the buffer and get the hash. */
        sha.Update(buffer, block_size);
        sha.GetHash(out, sizeof(*out));

        /* Set the validation bit, if the hash is for save data. */
        if (this->storage_type == fs::StorageType_SaveData) {
            SetValidationBit(out);
        }
    }

    Result IntegrityVerificationStorage::ReadBlockSignature(void *dst, size_t dst_size, s64 offset, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(this->verification_block_size)));
        AMS_ASSERT(util::IsAligned(size, static_cast<size_t>(this->verification_block_size)));

        /* Determine where to read the signature. */
        const s64 sign_offset = (offset >> this->verification_block_order) * HashSize;
        const auto sign_size  = static_cast<size_t>((size >> this->verification_block_order) * HashSize);
        AMS_ASSERT(dst_size >= sign_size);

        /* Create a guard in the event of failure. */
        auto clear_guard = SCOPE_GUARD { std::memset(dst, 0, sign_size); };

        /* Validate that we can read the signature. */
        s64 hash_size;
        R_TRY(this->hash_storage.GetSize(std::addressof(hash_size)));
        const bool range_valid = static_cast<s64>(sign_offset + sign_size) <= hash_size;
        AMS_ASSERT(range_valid);
        R_UNLESS(range_valid, fs::ResultOutOfRange());

        /* Read the signature. */
        R_TRY(this->hash_storage.Read(sign_offset, dst, sign_size));

        /* We succeeded. */
        clear_guard.Cancel();
        return ResultSuccess();
    }

    Result IntegrityVerificationStorage::WriteBlockSignature(const void *src, size_t src_size, s64 offset, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(src != nullptr);
        AMS_ASSERT(util::IsAligned(offset, static_cast<size_t>(this->verification_block_size)));

        /* Determine where to write the signature. */
        const s64 sign_offset = (offset >> this->verification_block_order) * HashSize;
        const auto sign_size  = static_cast<size_t>((size >> this->verification_block_order) * HashSize);
        AMS_ASSERT(src_size >= sign_size);

        /* Write the signature. */
        R_TRY(this->hash_storage.Write(sign_offset, src, sign_size));

        /* We succeeded. */
        return ResultSuccess();
    }

    Result IntegrityVerificationStorage::VerifyHash(const void *buf, BlockHash *hash) {
        /* Validate preconditions. */
        AMS_ASSERT(buf != nullptr);
        AMS_ASSERT(hash != nullptr);

        /* Get the comparison hash. */
        auto &cmp_hash = *hash;

        /* If save data, check if the data is uninitialized. */
        if (this->storage_type == fs::StorageType_SaveData) {
            bool is_cleared = false;
            R_TRY(this->IsCleared(std::addressof(is_cleared), cmp_hash));
            R_UNLESS(!is_cleared, fs::ResultClearedRealDataVerificationFailed());
        }

        /* Get the calculated hash. */
        BlockHash calc_hash;
        this->CalcBlockHash(std::addressof(calc_hash), buf);

        /* Check that the signatures are equal. */
        if (!crypto::IsSameBytes(std::addressof(cmp_hash), std::addressof(calc_hash), sizeof(BlockHash))) {
            /* Clear the comparison hash. */
            std::memset(std::addressof(cmp_hash), 0, sizeof(cmp_hash));

            /* Return the appropriate result. */
            if (this->is_real_data) {
                return fs::ResultUnclearedRealDataVerificationFailed();
            } else {
                return fs::ResultNonRealDataVerificationFailed();
            }
        }

        return ResultSuccess();
    }

    Result IntegrityVerificationStorage::IsCleared(bool *is_cleared, const BlockHash &hash) {
        /* Validate preconditions. */
        AMS_ASSERT(is_cleared != nullptr);
        AMS_ASSERT(this->storage_type == fs::StorageType_SaveData);

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
        return ResultSuccess();
    }

}
