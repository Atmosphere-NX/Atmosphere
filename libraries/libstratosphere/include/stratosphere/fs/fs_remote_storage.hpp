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
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_istorage_for_batch_read.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fs {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteStorage : public IStorage, public impl::Newable {
        NON_COPYABLE(RemoteStorage);
        NON_MOVEABLE(RemoteStorage);
        private:
            ::FsStorage m_base_storage;
        public:
            RemoteStorage(::FsStorage &s) : m_base_storage(s) { /* ... */}

            virtual ~RemoteStorage() { fsStorageClose(std::addressof(m_base_storage)); }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                R_RETURN(fsStorageRead(std::addressof(m_base_storage), offset, buffer, size));
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                R_RETURN(fsStorageWrite(std::addressof(m_base_storage), offset, buffer, size));
            }

            virtual Result Flush() override {
                R_RETURN(fsStorageFlush(std::addressof(m_base_storage)));
            }
            
            virtual Result SetSize(s64 size) override {
                R_RETURN(fsStorageSetSize(std::addressof(m_base_storage), size));
            }

            virtual Result GetSize(s64 *out_size) override {
                R_RETURN(fsStorageGetSize(std::addressof(m_base_storage), out_size));
            }

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* TODO: How to deal with this? */
                AMS_UNUSED(dst, dst_size, op_id, offset, size, src, src_size);
                R_THROW(fs::ResultUnsupportedOperation());
            }
    };
    
    class RemoteStorageForBatchRead : public IStorageForBatchRead, public impl::Newable {
        NON_COPYABLE(RemoteStorageForBatchRead);
        NON_MOVEABLE(RemoteStorageForBatchRead);
        private:
            ::FsStorageForBatchRead m_base_storage;
        public:
            RemoteStorageForBatchRead(::FsStorageForBatchRead &s) : m_base_storage(s) { /* ... */}

            virtual ~RemoteStorageForBatchRead() { fsStorageForBatchReadClose(std::addressof(m_base_storage)); }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                R_RETURN(fsStorageForBatchReadRead(std::addressof(m_base_storage), offset, buffer, size));
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                R_RETURN(fsStorageForBatchReadWrite(std::addressof(m_base_storage), offset, buffer, size));
            }

            virtual Result Flush() override {
                R_RETURN(fsStorageForBatchReadFlush(std::addressof(m_base_storage)));
            }
            
            virtual Result SetSize(s64 size) override {
                R_RETURN(fsStorageForBatchReadSetSize(std::addressof(m_base_storage), size));
            }

            virtual Result GetSize(s64 *out_size) override {
                R_RETURN(fsStorageForBatchReadGetSize(std::addressof(m_base_storage), out_size));
            }
            
            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* TODO: How to deal with this? */
                AMS_UNUSED(dst, dst_size, op_id, offset, size, src, src_size);
                R_THROW(fs::ResultUnsupportedOperation());
            }
            
            virtual Result BatchRead(void *out0, void *out1, void *out2, void *out3, void *out4, void *out5, void *out6, const void *in) override {
                /* TODO */
                AMS_UNUSED(out0, out1, out2, out3, out4, out5, out6, in);
                R_THROW(fs::ResultUnsupportedOperation());
            }
    };
    #endif

}
