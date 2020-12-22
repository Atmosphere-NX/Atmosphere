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

        template<typename T>
        constexpr ALWAYS_INLINE size_t GetRoundDownDifference(T x, size_t align) {
            return static_cast<size_t>(x - util::AlignDown(x, align));
        }

        template<typename T>
        constexpr ALWAYS_INLINE size_t GetRoundUpDifference(T x, size_t align) {
            return static_cast<size_t>(util::AlignUp(x, align) - x);
        }

        template<typename T>
        ALWAYS_INLINE size_t GetRoundUpDifference(T *x, size_t align) {
            return GetRoundUpDifference(reinterpret_cast<uintptr_t>(x), align);
        }

    }

    Result AlignmentMatchingStorageImpl::Read(fs::IStorage *base_storage, char *work_buf, size_t work_buf_size, size_t data_alignment, size_t buffer_alignment, s64 offset, char *buffer, size_t size) {
        /* Check preconditions. */
        AMS_ASSERT(work_buf_size >= data_alignment);

        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Determine extents. */
        char *aligned_core_buffer;
        s64 core_offset;
        size_t core_size;
        size_t buffer_gap;
        size_t offset_gap;
        s64 covered_offset;

        const size_t offset_round_up_difference = GetRoundUpDifference(offset, data_alignment);
        if (util::IsAligned(reinterpret_cast<uintptr_t>(buffer) + offset_round_up_difference, buffer_alignment)) {
            aligned_core_buffer = buffer + offset_round_up_difference;

            core_offset = util::AlignUp(offset, data_alignment);
            core_size   = (size < offset_round_up_difference) ? 0 : util::AlignDown(size - offset_round_up_difference, data_alignment);
            buffer_gap  = 0;
            offset_gap  = 0;

            covered_offset = core_size > 0 ? core_offset : offset;
        } else {
            const size_t buffer_round_up_difference = GetRoundUpDifference(buffer, buffer_alignment);

            aligned_core_buffer = buffer + buffer_round_up_difference;

            core_offset = util::AlignDown(offset, data_alignment);
            core_size   = (size < buffer_round_up_difference) ? 0 : util::AlignDown(size - buffer_round_up_difference, data_alignment);
            buffer_gap  = buffer_round_up_difference;
            offset_gap  = GetRoundDownDifference(offset, data_alignment);

            covered_offset = offset;
        }

        /* Read the core portion. */
        if (core_size > 0) {
            R_TRY(base_storage->Read(core_offset, aligned_core_buffer, core_size));

            if (offset_gap != 0 || buffer_gap != 0) {
                std::memmove(aligned_core_buffer - buffer_gap, aligned_core_buffer + offset_gap, core_size - offset_gap);
                core_size -= offset_gap;
            }
        }

        /* Handle the head portion. */
        if (offset < covered_offset) {
            const s64 head_offset  = util::AlignDown(offset, data_alignment);
            const size_t head_size = static_cast<size_t>(covered_offset - offset);

            AMS_ASSERT(GetRoundDownDifference(offset, data_alignment) + head_size <= work_buf_size);

            R_TRY(base_storage->Read(head_offset, work_buf, data_alignment));
            std::memcpy(buffer, work_buf + GetRoundDownDifference(offset, data_alignment), head_size);
        }

        /* Handle the tail portion. */
        s64 tail_offset = covered_offset + core_size;
        size_t remaining_tail_size = static_cast<size_t>((offset + size) - tail_offset);
        while (remaining_tail_size > 0) {
            const auto aligned_tail_offset = util::AlignDown(tail_offset, data_alignment);
            const auto cur_size = std::min(static_cast<size_t>(aligned_tail_offset + data_alignment - tail_offset), remaining_tail_size);
            R_TRY(base_storage->Read(aligned_tail_offset, work_buf, data_alignment));

            AMS_ASSERT((tail_offset - offset) + cur_size <= size);
            AMS_ASSERT((tail_offset - aligned_tail_offset) + cur_size <= data_alignment);
            std::memcpy(static_cast<char *>(buffer) + (tail_offset - offset), work_buf + (tail_offset - aligned_tail_offset), cur_size);

            remaining_tail_size -= cur_size;
            tail_offset         += cur_size;
        }

        return ResultSuccess();
    }

    Result AlignmentMatchingStorageImpl::Write(fs::IStorage *base_storage, char *work_buf, size_t work_buf_size, size_t data_alignment, size_t buffer_alignment, s64 offset, const char *buffer, size_t size) {
        /* Check preconditions. */
        AMS_ASSERT(work_buf_size >= data_alignment);

        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Determine extents. */
        const char *aligned_core_buffer;
        s64 core_offset;
        size_t core_size;
        s64 covered_offset;

        const size_t offset_round_up_difference = GetRoundUpDifference(offset, data_alignment);
        if (util::IsAligned(reinterpret_cast<uintptr_t>(buffer) + offset_round_up_difference, buffer_alignment)) {
            aligned_core_buffer = buffer + offset_round_up_difference;

            core_offset = util::AlignUp(offset, data_alignment);
            core_size   = (size < offset_round_up_difference) ? 0 : util::AlignDown(size - offset_round_up_difference, data_alignment);

            covered_offset = core_size > 0 ? core_offset : offset;
        } else {
            aligned_core_buffer = nullptr;

            core_offset = util::AlignDown(offset, data_alignment);
            core_size   = 0;

            covered_offset = offset;
        }

        /* Write the core portion. */
        if (core_size > 0) {
            R_TRY(base_storage->Write(core_offset, aligned_core_buffer, core_size));
        }

        /* Handle the head portion. */
        if (offset < covered_offset) {
            const s64 head_offset  = util::AlignDown(offset, data_alignment);
            const size_t head_size = static_cast<size_t>(covered_offset - offset);

            AMS_ASSERT((offset - head_offset) + head_size <= data_alignment);

            R_TRY(base_storage->Read(head_offset, work_buf, data_alignment));
            std::memcpy(work_buf + (offset - head_offset), buffer, head_size);
            R_TRY(base_storage->Write(head_offset, work_buf, data_alignment));
        }

        /* Handle the tail portion. */
        s64 tail_offset = covered_offset + core_size;
        size_t remaining_tail_size = static_cast<size_t>((offset + size) - tail_offset);
        while (remaining_tail_size > 0) {
            AMS_ASSERT(static_cast<size_t>(tail_offset - offset) < size);

            const auto aligned_tail_offset = util::AlignDown(tail_offset, data_alignment);
            const auto cur_size = std::min(static_cast<size_t>(aligned_tail_offset + data_alignment - tail_offset), remaining_tail_size);

            R_TRY(base_storage->Read(aligned_tail_offset, work_buf, data_alignment));
            std::memcpy(work_buf + GetRoundDownDifference(tail_offset, data_alignment), buffer + (tail_offset - offset), cur_size);
            R_TRY(base_storage->Write(aligned_tail_offset, work_buf, data_alignment));

            remaining_tail_size -= cur_size;
            tail_offset         += cur_size;
        }

        return ResultSuccess();
    }

    template<>
    Result AlignmentMatchingStorageInBulkRead<1>::Read(s64 offset, void *buffer, size_t size) {
        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        s64 bs_size = 0;
        R_TRY(this->GetSize(std::addressof(bs_size)));
        R_UNLESS(fs::IStorage::CheckAccessRange(offset, size, bs_size), fs::ResultOutOfRange());

        /* Determine extents. */
        const auto offset_end         = offset + static_cast<s64>(size);
        const auto aligned_offset     = util::AlignDown(offset, this->data_align);
        const auto aligned_offset_end = util::AlignUp(offset_end, this->data_align);
        const auto aligned_size       = static_cast<size_t>(aligned_offset_end - aligned_offset);

        /* If we aren't aligned, we need to allocate a buffer. */
        PooledBuffer pooled_buffer;
        if (aligned_offset != offset || aligned_size != size) {
            if (aligned_size <= pooled_buffer.GetAllocatableSizeMax()) {
                pooled_buffer.Allocate(aligned_size, this->data_align);

                if (aligned_size <= pooled_buffer.GetSize()) {
                    R_TRY(this->base_storage->Read(aligned_offset, pooled_buffer.GetBuffer(), aligned_size));
                    std::memcpy(buffer, pooled_buffer.GetBuffer() + (offset - aligned_offset), size);
                    return ResultSuccess();
                } else {
                    pooled_buffer.Shrink(this->data_align);
                }
            } else {
                pooled_buffer.Allocate(this->data_align, this->data_align);
            }

            AMS_ASSERT(pooled_buffer.GetSize() >= static_cast<size_t>(this->data_align));
        }

        /* Determine read extents for the aligned portion. */
        const auto core_offset     = util::AlignUp(offset, this->data_align);
        const auto core_offset_end = util::AlignDown(offset_end, this->data_align);

        /* Handle any data before the aligned portion. */
        if (offset < core_offset) {
            const auto head_size = static_cast<size_t>(core_offset - offset);
            AMS_ASSERT(head_size < size);
            R_TRY(this->base_storage->Read(aligned_offset, pooled_buffer.GetBuffer(), this->data_align));
            std::memcpy(buffer, pooled_buffer.GetBuffer() + (offset - aligned_offset), head_size);
        }

        /* Handle the aligned portion. */
        if (core_offset < core_offset_end) {
            const auto core_buffer = static_cast<char *>(buffer) + (core_offset - offset);
            const auto core_size   = static_cast<size_t>(core_offset_end - core_offset);

            R_TRY(this->base_storage->Read(core_offset, core_buffer, core_size));
        }

        /* Handle any data after the aligned portion. */
        if (core_offset_end < offset_end) {
            const auto tail_size = static_cast<size_t>(offset_end - core_offset_end);
            R_TRY(this->base_storage->Read(core_offset_end, pooled_buffer.GetBuffer(), this->data_align));
            std::memcpy(buffer, pooled_buffer.GetBuffer(), tail_size);
        }

        return ResultSuccess();
    }

}
