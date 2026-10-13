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
#include <stratosphere/fs/fs_query_range.hpp>
#include <stratosphere/fssystem/fssystem_utility.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_istorage.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_istorage_for_batch_read.hpp>

namespace ams::fs {

    class IStorage;
    class IStorageForBatchRead;

}

namespace ams::fssrv::impl {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class StorageInterfaceAdapter {
        NON_COPYABLE(StorageInterfaceAdapter);
        private:
            std::shared_ptr<fs::IStorage> m_base_storage;
        public:
            explicit StorageInterfaceAdapter(std::shared_ptr<fs::IStorage> &&storage) : m_base_storage(std::move(storage)) { /* ... */ }
        public:
            /* Command API. */
            Result Read(s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size);
            Result Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size);
            Result Flush();
            Result SetSize(s64 size);
            Result GetSize(ams::sf::Out<s64> out);
            Result OperateRange(ams::sf::Out<fs::StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size);
    };
    static_assert(fssrv::sf::IsIStorage<StorageInterfaceAdapter>);
    
    class StorageInterfaceAdapterForBatchRead : public StorageInterfaceAdapter {
        NON_COPYABLE(StorageInterfaceAdapterForBatchRead);
        public:
            explicit StorageInterfaceAdapterForBatchRead(std::shared_ptr<fs::IStorageForBatchRead> &&storage) : StorageInterfaceAdapter(storage) { /* ... */ }
        public:
            /* Command API. */
            Result BatchRead(const ams::sf::OutNonSecureBuffer &out0, const ams::sf::OutNonSecureBuffer &out1, const ams::sf::OutNonSecureBuffer &out2, const ams::sf::OutNonSecureBuffer &out3, const ams::sf::OutNonSecureBuffer &out4, const ams::sf::OutNonSecureBuffer &out5, const ams::sf::OutNonSecureBuffer &out6, const ams::sf::InBuffer &in);
    };
    static_assert(fssrv::sf::IsIStorageForBatchRead<StorageInterfaceAdapterForBatchRead>);

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteStorage {
        NON_COPYABLE(RemoteStorage);
        NON_MOVEABLE(RemoteStorage);
        private:
            ::FsStorage m_base_storage;
        public:
            RemoteStorage(::FsStorage &s) : m_base_storage(s) { /* ... */}

            virtual ~RemoteStorage() { fsStorageClose(std::addressof(m_base_storage)); }
        public:
            Result Read(s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size) {
                R_RETURN(fsStorageRead(std::addressof(m_base_storage), offset, buffer.GetPointer(), size));
            }

            Result Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size) {
                R_RETURN(fsStorageWrite(std::addressof(m_base_storage), offset, buffer.GetPointer(), size));
            }

            Result Flush(){
                R_RETURN(fsStorageFlush(std::addressof(m_base_storage)));
            }

            Result SetSize(s64 size) {
                R_RETURN(fsStorageSetSize(std::addressof(m_base_storage), size));
            }

            Result GetSize(ams::sf::Out<s64> out) {
                R_RETURN(fsStorageGetSize(std::addressof(m_base_storage), out.GetPointer()));
            }

            Result OperateRange(ams::sf::Out<fs::StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
                static_assert(sizeof(::FsRangeInfo) == sizeof(fs::StorageQueryRangeInfo));
                R_RETURN(fsStorageOperateRange(std::addressof(m_base_storage), static_cast<::FsOperationId>(op_id), offset, size, reinterpret_cast<::FsRangeInfo *>(out.GetPointer())));
            }
    };
    static_assert(fssrv::sf::IsIStorage<RemoteStorage>);
    
    class RemoteStorageForBatchRead {
        NON_COPYABLE(RemoteStorageForBatchRead);
        NON_MOVEABLE(RemoteStorageForBatchRead);
        private:
            ::FsStorageForBatchRead m_base_storage;
        public:
            RemoteStorageForBatchRead(::FsStorageForBatchRead &s) : m_base_storage(s) { /* ... */}

            virtual ~RemoteStorageForBatchRead() { fsStorageForBatchReadClose(std::addressof(m_base_storage)); }
        public:
            Result Read(s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size) {
                R_RETURN(fsStorageForBatchReadRead(std::addressof(m_base_storage), offset, buffer.GetPointer(), size));
            }

            Result Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size) {
                R_RETURN(fsStorageForBatchReadWrite(std::addressof(m_base_storage), offset, buffer.GetPointer(), size));
            }

            Result Flush(){
                R_RETURN(fsStorageForBatchReadFlush(std::addressof(m_base_storage)));
            }

            Result SetSize(s64 size) {
                R_RETURN(fsStorageForBatchReadSetSize(std::addressof(m_base_storage), size));
            }

            Result GetSize(ams::sf::Out<s64> out) {
                R_RETURN(fsStorageForBatchReadGetSize(std::addressof(m_base_storage), out.GetPointer()));
            }

            Result OperateRange(ams::sf::Out<fs::StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
                static_assert(sizeof(::FsRangeInfo) == sizeof(fs::StorageQueryRangeInfo));
                R_RETURN(fsStorageForBatchReadOperateRange(std::addressof(m_base_storage), static_cast<::FsOperationId>(op_id), offset, size, reinterpret_cast<::FsRangeInfo *>(out.GetPointer())));
            }
            
            Result BatchRead(const ams::sf::OutNonSecureBuffer &out0, const ams::sf::OutNonSecureBuffer &out1, const ams::sf::OutNonSecureBuffer &out2, const ams::sf::OutNonSecureBuffer &out3, const ams::sf::OutNonSecureBuffer &out4, const ams::sf::OutNonSecureBuffer &out5, const ams::sf::OutNonSecureBuffer &out6, const ams::sf::InBuffer &in) {
                R_RETURN(fsStorageForBatchReadBatchRead(std::addressof(m_base_storage), out0.GetPointer(), out1.GetPointer(), out2.GetPointer(), out3.GetPointer(), out4.GetPointer(), out5.GetPointer(), out6.GetPointer(), in.GetPointer()));
            }
    };
    static_assert(fssrv::sf::IsIStorageForBatchRead<RemoteStorageForBatchRead>);
    #endif

}
