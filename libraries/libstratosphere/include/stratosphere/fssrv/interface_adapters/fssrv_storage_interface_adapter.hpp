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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_query_range.hpp>
#include <stratosphere/fssystem/fssystem_utility.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_istorage.hpp>

namespace ams::fs {

    class IStorage;

}

namespace ams::fssrv::impl {

    class StorageInterfaceAdapter final {
        NON_COPYABLE(StorageInterfaceAdapter);
        private:
            /* TODO: Nintendo uses fssystem::AsynchronousAccessStorage here. */
            std::shared_ptr<fs::IStorage> base_storage;
            std::unique_lock<fssystem::SemaphoreAdapter> open_count_semaphore;
            os::ReadWriteLock invalidation_lock;
            /* TODO: DataStorageContext. */
            bool deep_retry_enabled = false;
        public:
            StorageInterfaceAdapter(fs::IStorage *storage);
            StorageInterfaceAdapter(std::unique_ptr<fs::IStorage> storage);
            explicit StorageInterfaceAdapter(std::shared_ptr<fs::IStorage> &&storage);
            /* TODO: Other constructors. */

            ~StorageInterfaceAdapter();
        private:
            std::optional<std::shared_lock<os::ReadWriteLock>> AcquireCacheInvalidationReadLock();
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

}
