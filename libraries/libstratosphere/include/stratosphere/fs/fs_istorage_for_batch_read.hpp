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
#pragma once
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_operate_range.hpp>

namespace ams::fs {

    class IStorageForBatchRead : public IStorage {
        public:
            virtual ~IStorageForBatchRead() { /* ... */ }
            
            virtual Result BatchRead(void *out0, void *out1, void *out2, void *out3, void *out4, void *out5, void *out6, const void *in) = 0;
    };

    class ReadOnlyStorageAdapterForBatchRead : public IStorageForBatchRead {
        private:
            std::shared_ptr<IStorageForBatchRead> m_shared_storage;
            std::unique_ptr<IStorageForBatchRead> m_unique_storage;
            IStorageForBatchRead *m_storage;
        public:
            explicit ReadOnlyStorageAdapterForBatchRead(IStorageForBatchRead *s) : m_unique_storage(s) {
                m_storage = m_unique_storage.get();
            }
            explicit ReadOnlyStorageAdapterForBatchRead(std::shared_ptr<IStorageForBatchRead> s) : m_shared_storage(s) {
                m_storage = m_shared_storage.get();
            }
            explicit ReadOnlyStorageAdapterForBatchRead(std::unique_ptr<IStorageForBatchRead> s) : m_unique_storage(std::move(s)) {
                m_storage = m_unique_storage.get();
            }
            explicit ReadOnlyStorageAdapterForBatchRead(IStorage *s) : m_unique_storage(static_cast<IStorageForBatchRead *>(s)) {
                m_storage = m_unique_storage.get();
            }
            explicit ReadOnlyStorageAdapterForBatchRead(std::shared_ptr<IStorage> s) : m_shared_storage(std::static_pointer_cast<IStorageForBatchRead>(s)) {
                m_storage = m_shared_storage.get();
            }
            explicit ReadOnlyStorageAdapterForBatchRead(std::unique_ptr<IStorage> s) : m_unique_storage(std::unique_ptr<IStorageForBatchRead>(static_cast<IStorageForBatchRead *>(s.release()))) {
                m_storage = m_unique_storage.get();
            }

            virtual ~ReadOnlyStorageAdapterForBatchRead() { /* ... */ }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                R_RETURN(m_storage->Read(offset, buffer, size));
            }
            
            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* TODO: Better result? Is it possible to get a more specific one? */
                AMS_UNUSED(offset, buffer, size);
                R_THROW(fs::ResultUnsupportedOperation());
            }

            virtual Result Flush() override {
                R_RETURN(m_storage->Flush());
            }
            
            virtual Result SetSize(s64 size) override {
                /* TODO: Better result? Is it possible to get a more specific one? */
                AMS_UNUSED(size);
                R_THROW(fs::ResultUnsupportedOperation());
            }
            
            virtual Result GetSize(s64 *out) override {
                R_RETURN(m_storage->GetSize(out));
            }

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                R_RETURN(m_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
            }
            
            virtual Result BatchRead(void *out0, void *out1, void *out2, void *out3, void *out4, void *out5, void *out6, const void *in) override {
                /* TODO */
                AMS_UNUSED(out0, out1, out2, out3, out4, out5, out6, in);
                R_THROW(fs::ResultUnsupportedOperation());
            }
    };

    template<typename T>
    concept PointerToStorageForBatchRead = ::ams::util::RawOrSmartPointerTo<T, ::ams::fs::IStorageForBatchRead>;

}
