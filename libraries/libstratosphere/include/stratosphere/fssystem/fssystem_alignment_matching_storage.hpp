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
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fssystem/fssystem_alignment_matching_storage_impl.hpp>
#include <stratosphere/fssystem/fssystem_pooled_buffer.hpp>

namespace ams::fssystem {

    template<size_t _DataAlign, size_t _BufferAlign>
    class AlignmentMatchingStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(AlignmentMatchingStorage);
        NON_MOVEABLE(AlignmentMatchingStorage);
        public:
            static constexpr size_t DataAlign   = _DataAlign;
            static constexpr size_t BufferAlign = _BufferAlign;

            static constexpr size_t DataAlignMax = 0x200;
            static_assert(DataAlign <= DataAlignMax);
            static_assert(util::IsPowerOfTwo(DataAlign));
            static_assert(util::IsPowerOfTwo(BufferAlign));
        private:
            std::shared_ptr<fs::IStorage> shared_base_storage;
            fs::IStorage * const base_storage;
            s64 base_storage_size;
            bool is_base_storage_size_dirty;
        public:
            explicit AlignmentMatchingStorage(fs::IStorage *bs) : base_storage(bs), is_base_storage_size_dirty(true) {
                /* ... */
            }

            explicit AlignmentMatchingStorage(std::shared_ptr<fs::IStorage> bs) : shared_base_storage(bs), base_storage(shared_base_storage.get()), is_base_storage_size_dirty(true) {
                /* ... */
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Allocate a work buffer on stack. */
                __attribute__((aligned(DataAlignMax))) char work_buf[DataAlign];
                static_assert(util::IsAligned(alignof(work_buf), BufferAlign));

                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckAccessRange(offset, size, bs_size), fs::ResultOutOfRange());

                return AlignmentMatchingStorageImpl::Read(this->base_storage, work_buf, sizeof(work_buf), DataAlign, BufferAlign, offset, static_cast<char *>(buffer), size);
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* Allocate a work buffer on stack. */
                __attribute__((aligned(DataAlignMax))) char work_buf[DataAlign];
                static_assert(util::IsAligned(alignof(work_buf), BufferAlign));

                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckAccessRange(offset, size, bs_size), fs::ResultOutOfRange());

                return AlignmentMatchingStorageImpl::Write(this->base_storage, work_buf, sizeof(work_buf), DataAlign, BufferAlign, offset, static_cast<const char *>(buffer), size);
            }

            virtual Result Flush() override {
                return this->base_storage->Flush();
            }

            virtual Result SetSize(s64 size) override {
                ON_SCOPE_EXIT { this->is_base_storage_size_dirty = true; };
                return this->base_storage->SetSize(util::AlignUp(size, DataAlign));
            }

            virtual Result GetSize(s64 *out) override {
                AMS_ASSERT(out != nullptr);

                if (this->is_base_storage_size_dirty) {
                    s64 size;
                    R_TRY(this->base_storage->GetSize(std::addressof(size)));

                    this->base_storage_size = size;
                    this->is_base_storage_size_dirty = false;
                }

                *out = this->base_storage_size;
                return ResultSuccess();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Get the base storage size. */
                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckOffsetAndSize(offset, size), fs::ResultOutOfRange());

                /* Operate on the base storage. */
                const auto valid_size         = std::min(size, bs_size - offset);
                const auto aligned_offset     = util::AlignDown(offset, DataAlign);
                const auto aligned_offset_end = util::AlignUp(offset + valid_size, DataAlign);
                const auto aligned_size       = aligned_offset_end - aligned_offset;

                return this->base_storage->OperateRange(dst, dst_size, op_id, aligned_offset, aligned_size, src, src_size);
            }
    };

    template<size_t _BufferAlign>
    class AlignmentMatchingStoragePooledBuffer : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(AlignmentMatchingStoragePooledBuffer);
        NON_MOVEABLE(AlignmentMatchingStoragePooledBuffer);
        public:
            static constexpr size_t BufferAlign = _BufferAlign;

            static_assert(util::IsPowerOfTwo(BufferAlign));
        private:
            fs::IStorage * const base_storage;
            s64 base_storage_size;
            size_t data_align;
            bool is_base_storage_size_dirty;
        public:
            explicit AlignmentMatchingStoragePooledBuffer(fs::IStorage *bs, size_t da) : base_storage(bs), data_align(da), is_base_storage_size_dirty(true) {
                AMS_ASSERT(util::IsPowerOfTwo(da));
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckAccessRange(offset, size, bs_size), fs::ResultOutOfRange());

                /* Allocate a pooled buffer. */
                PooledBuffer pooled_buffer;
                pooled_buffer.AllocateParticularlyLarge(this->data_align, this->data_align);

                return AlignmentMatchingStorageImpl::Read(this->base_storage, pooled_buffer.GetBuffer(), pooled_buffer.GetSize(), this->data_align, BufferAlign, offset, static_cast<char *>(buffer), size);
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckAccessRange(offset, size, bs_size), fs::ResultOutOfRange());

                /* Allocate a pooled buffer. */
                PooledBuffer pooled_buffer;
                pooled_buffer.AllocateParticularlyLarge(this->data_align, this->data_align);

                return AlignmentMatchingStorageImpl::Write(this->base_storage, pooled_buffer.GetBuffer(), pooled_buffer.GetSize(), this->data_align, BufferAlign, offset, static_cast<const char *>(buffer), size);
            }

            virtual Result Flush() override {
                return this->base_storage->Flush();
            }

            virtual Result SetSize(s64 size) override {
                ON_SCOPE_EXIT { this->is_base_storage_size_dirty = true; };
                return this->base_storage->SetSize(util::AlignUp(size, this->data_align));
            }

            virtual Result GetSize(s64 *out) override {
                AMS_ASSERT(out != nullptr);

                if (this->is_base_storage_size_dirty) {
                    s64 size;
                    R_TRY(this->base_storage->GetSize(std::addressof(size)));

                    this->base_storage_size = size;
                    this->is_base_storage_size_dirty = false;
                }

                *out = this->base_storage_size;
                return ResultSuccess();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Get the base storage size. */
                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckOffsetAndSize(offset, size), fs::ResultOutOfRange());

                /* Operate on the base storage. */
                const auto valid_size         = std::min(size, bs_size - offset);
                const auto aligned_offset     = util::AlignDown(offset, this->data_align);
                const auto aligned_offset_end = util::AlignUp(offset + valid_size, this->data_align);
                const auto aligned_size       = aligned_offset_end - aligned_offset;

                return this->base_storage->OperateRange(dst, dst_size, op_id, aligned_offset, aligned_size, src, src_size);
            }
    };

    template<size_t _BufferAlign>
    class AlignmentMatchingStorageInBulkRead : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(AlignmentMatchingStorageInBulkRead);
        NON_MOVEABLE(AlignmentMatchingStorageInBulkRead);
        public:
            static constexpr size_t BufferAlign = _BufferAlign;

            static_assert(util::IsPowerOfTwo(BufferAlign));
        private:
            std::shared_ptr<fs::IStorage> shared_base_storage;
            fs::IStorage * const base_storage;
            s64 base_storage_size;
            size_t data_align;
        public:
            explicit AlignmentMatchingStorageInBulkRead(fs::IStorage *bs, size_t da) : shared_base_storage(), base_storage(bs), base_storage_size(-1), data_align(da) {
                AMS_ASSERT(util::IsPowerOfTwo(this->data_align));
            }

            explicit AlignmentMatchingStorageInBulkRead(std::shared_ptr<fs::IStorage> bs, size_t da) : shared_base_storage(bs), base_storage(shared_base_storage.get()), base_storage_size(-1), data_align(da) {
                AMS_ASSERT(util::IsPowerOfTwo(da));
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override;

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckAccessRange(offset, size, bs_size), fs::ResultOutOfRange());

                /* Allocate a pooled buffer. */
                PooledBuffer pooled_buffer(this->data_align, this->data_align);
                return AlignmentMatchingStorageImpl::Write(this->base_storage, pooled_buffer.GetBuffer(), pooled_buffer.GetSize(), this->data_align, BufferAlign, offset, static_cast<const char *>(buffer), size);
            }

            virtual Result Flush() override {
                return this->base_storage->Flush();
            }

            virtual Result SetSize(s64 size) override {
                ON_SCOPE_EXIT { this->base_storage_size = -1; };
                return this->base_storage->SetSize(util::AlignUp(size, this->data_align));
            }

            virtual Result GetSize(s64 *out) override {
                AMS_ASSERT(out != nullptr);

                if (this->base_storage_size < 0) {
                    s64 size;
                    R_TRY(this->base_storage->GetSize(std::addressof(size)));

                    this->base_storage_size = size;
                }

                *out = this->base_storage_size;
                return ResultSuccess();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Succeed if zero size. */
                R_SUCCEED_IF(size == 0);

                /* Get the base storage size. */
                s64 bs_size = 0;
                R_TRY(this->GetSize(std::addressof(bs_size)));
                R_UNLESS(fs::IStorage::CheckOffsetAndSize(offset, size), fs::ResultOutOfRange());

                /* Operate on the base storage. */
                const auto valid_size         = std::min(size, bs_size - offset);
                const auto aligned_offset     = util::AlignDown(offset, this->data_align);
                const auto aligned_offset_end = util::AlignUp(offset + valid_size, this->data_align);
                const auto aligned_size       = aligned_offset_end - aligned_offset;

                return this->base_storage->OperateRange(dst, dst_size, op_id, aligned_offset, aligned_size, src, src_size);
            }
    };

}
