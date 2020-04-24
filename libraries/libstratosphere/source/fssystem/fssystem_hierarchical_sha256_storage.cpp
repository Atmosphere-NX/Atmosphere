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
#include "fssystem_hierarchical_sha256_storage.hpp"

namespace ams::fssystem {

    namespace {

        s32 Log2(s32 value) {
            AMS_ASSERT(value > 0);
            AMS_ASSERT(util::IsPowerOfTwo(value));

            s32 log = 0;
            while ((value >>= 1) > 0) {
                ++log;
            }
            return log;
        }

    }

    Result HierarchicalSha256Storage::Initialize(IStorage **base_storages, s32 layer_count, size_t htbs, void *hash_buf, size_t hash_buf_size) {
        /* Validate preconditions. */
        AMS_ASSERT(layer_count == LayerCount);
        AMS_ASSERT(util::IsPowerOfTwo(htbs));
        AMS_ASSERT(hash_buf != nullptr);

        /* Set size tracking members. */
        this->hash_target_block_size = htbs;
        this->log_size_ratio         = Log2(this->hash_target_block_size / HashSize);

        /* Get the base storage size. */
        R_TRY(base_storages[2]->GetSize(std::addressof(this->base_storage_size)));
        {
            auto size_guard = SCOPE_GUARD { this->base_storage_size = 0; };
            R_UNLESS(this->base_storage_size <= static_cast<s64>(HashSize) << log_size_ratio << log_size_ratio, fs::ResultHierarchicalSha256BaseStorageTooLarge());
            size_guard.Cancel();
        }

        /* Set hash buffer tracking members. */
        this->base_storage     = base_storages[2];
        this->hash_buffer      = static_cast<char *>(hash_buf);
        this->hash_buffer_size = hash_buf_size;

        /* Read the master hash. */
        u8 master_hash[HashSize];
        R_TRY(base_storages[0]->Read(0, master_hash, HashSize));

        /* Read and validate the data being hashed. */
        s64 hash_storage_size;
        R_TRY(base_storages[1]->GetSize(std::addressof(hash_storage_size)));
        AMS_ASSERT(util::IsAligned(hash_storage_size, HashSize));
        AMS_ASSERT(hash_storage_size <= this->hash_target_block_size);
        AMS_ASSERT(hash_storage_size <= static_cast<s64>(this->hash_buffer_size));

        R_TRY(base_storages[1]->Read(0, this->hash_buffer, static_cast<size_t>(hash_storage_size)));

        /* Calculate and verify the master hash. */
        u8 calc_hash[HashSize];
        crypto::GenerateSha256Hash(calc_hash, sizeof(calc_hash), this->hash_buffer, static_cast<size_t>(hash_storage_size));
        R_UNLESS(crypto::IsSameBytes(master_hash, calc_hash, HashSize), fs::ResultHierarchicalSha256HashVerificationFailed());

        return ResultSuccess();
    }

    Result HierarchicalSha256Storage::Read(s64 offset, void *buffer, size_t size) {
        /* Succeed if zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Validate that we have a buffer to read into. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Validate preconditions. */
        R_UNLESS(util::IsAligned(offset, this->hash_target_block_size), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size,   this->hash_target_block_size), fs::ResultInvalidArgument());

        /* Read the data. */
        const size_t reduced_size = static_cast<size_t>(std::min<s64>(this->base_storage_size, util::AlignUp(offset + size, this->hash_target_block_size) - offset));
        R_TRY(this->base_storage->Read(offset, buffer, reduced_size));

        /* Temporarily increase our thread priority. */
        ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

        /* Setup tracking variables. */
        auto cur_offset     = offset;
        auto remaining_size = reduced_size;
        while (remaining_size > 0) {
            /* Generate the hash of the region we're validating. */
            u8 hash[HashSize];
            const auto cur_size = static_cast<size_t>(std::min<s64>(this->hash_target_block_size, remaining_size));
            crypto::GenerateSha256Hash(hash, sizeof(hash), static_cast<u8 *>(buffer) + (cur_offset - offset), cur_size);

            AMS_ASSERT(static_cast<size_t>(cur_offset >> this->log_size_ratio) < this->hash_buffer_size);

            /* Check the hash. */
            {
                std::scoped_lock lk(this->mutex);
                auto clear_guard = SCOPE_GUARD { std::memset(buffer, 0, size); };

                R_UNLESS(crypto::IsSameBytes(hash, std::addressof(this->hash_buffer[cur_offset >> this->log_size_ratio]), HashSize), fs::ResultHierarchicalSha256HashVerificationFailed());

                clear_guard.Cancel();
            }

            /* Advance. */
            cur_offset     += cur_size;
            remaining_size -= cur_size;
        }

        return ResultSuccess();
    }

    Result HierarchicalSha256Storage::Write(s64 offset, const void *buffer, size_t size) {
        /* Succeed if zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Validate that we have a buffer to read into. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Validate preconditions. */
        R_UNLESS(util::IsAligned(offset, this->hash_target_block_size), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size,   this->hash_target_block_size), fs::ResultInvalidArgument());

        /* Setup tracking variables. */
        const size_t reduced_size = static_cast<size_t>(std::min<s64>(this->base_storage_size, util::AlignUp(offset + size, this->hash_target_block_size) - offset));
        auto cur_offset     = offset;
        auto remaining_size = reduced_size;
        while (remaining_size > 0) {
            /* Generate the hash of the region we're validating. */
            u8 hash[HashSize];
            const auto cur_size = static_cast<size_t>(std::min<s64>(this->hash_target_block_size, remaining_size));
            {
                /* Temporarily increase our thread priority. */
                ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);
                crypto::GenerateSha256Hash(hash, sizeof(hash), static_cast<const u8 *>(buffer) + (cur_offset - offset), cur_size);
            }

            /* Write the data. */
            R_TRY(this->base_storage->Write(cur_offset, static_cast<const u8 *>(buffer) + (cur_offset - offset), cur_size));

            /* Write the hash. */
            {
                std::scoped_lock lk(this->mutex);
                std::memcpy(std::addressof(this->hash_buffer[cur_offset >> this->log_size_ratio]), hash, HashSize);
            }

            /* Advance. */
            cur_offset     += cur_size;
            remaining_size -= cur_size;
        }

        return ResultSuccess();
    }

    Result HierarchicalSha256Storage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        /* Succeed if zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Validate preconditions. */
        R_UNLESS(util::IsAligned(offset, this->hash_target_block_size), fs::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(size,   this->hash_target_block_size), fs::ResultInvalidArgument());

        /* Determine size to use. */
        const auto reduced_size = std::min<s64>(this->base_storage_size, util::AlignUp(offset + size, this->hash_target_block_size) - offset);

        /* Operate on the base storage. */
        return this->base_storage->OperateRange(dst, dst_size, op_id, offset, reduced_size, src, src_size);
    }

}
