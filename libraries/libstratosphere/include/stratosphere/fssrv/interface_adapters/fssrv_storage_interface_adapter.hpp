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

namespace ams::fs {

    class IStorage;

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
    #endif

}
