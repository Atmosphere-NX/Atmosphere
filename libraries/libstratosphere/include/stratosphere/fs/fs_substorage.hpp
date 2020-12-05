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
#include "impl/fs_newable.hpp"
#include "fs_istorage.hpp"

namespace ams::fs {

    class SubStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        private:
            std::shared_ptr<IStorage> shared_base_storage;
            fs::IStorage *base_storage;
            s64 offset;
            s64 size;
            bool resizable;
        private:
            constexpr bool IsValid() const {
                return this->base_storage != nullptr;
            }
        public:
            SubStorage() : shared_base_storage(), base_storage(nullptr), offset(0), size(0), resizable(false) { /* ... */ }

            SubStorage(const SubStorage &rhs) : shared_base_storage(), base_storage(rhs.base_storage), offset(rhs.offset), size(rhs.size), resizable(rhs.resizable) { /* ... */}
            SubStorage &operator=(const SubStorage &rhs) {
                if (this != std::addressof(rhs)) {
                    this->base_storage        = rhs.base_storage;
                    this->offset              = rhs.offset;
                    this->size                = rhs.size;
                    this->resizable           = rhs.resizable;
                }
                return *this;
            }

            SubStorage(IStorage *storage, s64 o, s64 sz) : shared_base_storage(), base_storage(storage), offset(o), size(sz), resizable(false) {
                AMS_ABORT_UNLESS(this->IsValid());
                AMS_ABORT_UNLESS(this->offset  >= 0);
                AMS_ABORT_UNLESS(this->size    >= 0);
            }

            SubStorage(std::shared_ptr<IStorage> storage, s64 o, s64 sz) : shared_base_storage(storage), base_storage(storage.get()), offset(o), size(sz), resizable(false) {
                AMS_ABORT_UNLESS(this->IsValid());
                AMS_ABORT_UNLESS(this->offset  >= 0);
                AMS_ABORT_UNLESS(this->size    >= 0);
            }

            SubStorage(SubStorage *sub, s64 o, s64 sz) : shared_base_storage(), base_storage(sub->base_storage), offset(o + sub->offset), size(sz), resizable(false) {
                AMS_ABORT_UNLESS(this->IsValid());
                AMS_ABORT_UNLESS(this->offset  >= 0);
                AMS_ABORT_UNLESS(this->size    >= 0);
                AMS_ABORT_UNLESS(sub->size     >= o + sz);
            }

        public:
            void SetResizable(bool rsz) {
                this->resizable = rsz;
            }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(),                                  fs::ResultNotInitialized());

                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);


                /* Validate arguments and read. */
                R_UNLESS(buffer != nullptr,                                    fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());
                return this->base_storage->Read(this->offset + offset, buffer, size);
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override{
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(),                                  fs::ResultNotInitialized());

                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments and write. */
                R_UNLESS(buffer != nullptr,                                    fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());
                return this->base_storage->Write(this->offset + offset, buffer, size);
            }

            virtual Result Flush() override {
                R_UNLESS(this->IsValid(), fs::ResultNotInitialized());
                return this->base_storage->Flush();
            }

            virtual Result SetSize(s64 size) override {
                /* Ensure we're initialized and validate arguments. */
                R_UNLESS(this->IsValid(),                                  fs::ResultNotInitialized());
                R_UNLESS(this->resizable,                                  fs::ResultUnsupportedOperationInSubStorageA());
                R_UNLESS(IStorage::CheckOffsetAndSize(this->offset, size), fs::ResultInvalidSize());

                /* Ensure that we're allowed to set size. */
                s64 cur_size;
                R_TRY(this->base_storage->GetSize(std::addressof(cur_size)));
                R_UNLESS(cur_size == this->offset + this->size, fs::ResultUnsupportedOperationInSubStorageB());

                /* Set the size. */
                R_TRY(this->base_storage->SetSize(this->offset + size));

                this->size = size;
                return ResultSuccess();
            }

            virtual Result GetSize(s64 *out) override {
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(), fs::ResultNotInitialized());

                *out = this->size;
                return ResultSuccess();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Ensure we're initialized. */
                R_UNLESS(this->IsValid(),                              fs::ResultNotInitialized());

                /* Succeed immediately on zero-sized operation. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments and operate. */
                R_UNLESS(IStorage::CheckOffsetAndSize(offset, size), fs::ResultOutOfRange());
                return this->base_storage->OperateRange(dst, dst_size, op_id, this->offset + offset, size, src, src_size);
            }

            using IStorage::OperateRange;
    };

}
