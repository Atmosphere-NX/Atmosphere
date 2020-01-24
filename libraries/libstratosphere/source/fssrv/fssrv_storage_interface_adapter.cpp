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
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

namespace ams::fssrv::impl {

    StorageInterfaceAdapter::StorageInterfaceAdapter(fs::IStorage *storage) : base_storage(storage) {
        /* ... */
    }

    StorageInterfaceAdapter::StorageInterfaceAdapter(std::unique_ptr<fs::IStorage> storage) : base_storage(storage.release()) {
        /* ... */
    }

    StorageInterfaceAdapter::StorageInterfaceAdapter(std::shared_ptr<fs::IStorage> &&storage) : base_storage(std::move(storage)) {
        /* ... */
    }

    StorageInterfaceAdapter::~StorageInterfaceAdapter() {
        /* ... */
    }

    std::optional<std::shared_lock<os::ReadWriteLock>> StorageInterfaceAdapter::AcquireCacheInvalidationReadLock() {
        std::optional<std::shared_lock<os::ReadWriteLock>> lock;
        if (this->deep_retry_enabled) {
            lock.emplace(this->invalidation_lock);
        }
        return lock;
    }

    Result StorageInterfaceAdapter::Read(s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size) {
        /* TODO: N retries on ResultDataCorrupted, we may want to eventually. */
        /* TODO: Deep retry */
        R_UNLESS(offset >= 0, fs::ResultInvalidOffset());
        R_UNLESS(size >= 0,   fs::ResultInvalidSize());
        return this->base_storage->Read(offset, buffer.GetPointer(), size);
    }

    Result StorageInterfaceAdapter::Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size)  {
        /* TODO: N increases thread priority temporarily when writing. We may want to eventually. */
        R_UNLESS(offset >= 0, fs::ResultInvalidOffset());
        R_UNLESS(size >= 0,   fs::ResultInvalidSize());

        auto read_lock = this->AcquireCacheInvalidationReadLock();
        return this->base_storage->Write(offset, buffer.GetPointer(), size);
    }

    Result StorageInterfaceAdapter::Flush()  {
        auto read_lock = this->AcquireCacheInvalidationReadLock();
        return this->base_storage->Flush();
    }

    Result StorageInterfaceAdapter::SetSize(s64 size)  {
        R_UNLESS(size >= 0, fs::ResultInvalidSize());
        auto read_lock = this->AcquireCacheInvalidationReadLock();
        return this->base_storage->SetSize(size);
    }

    Result StorageInterfaceAdapter::GetSize(ams::sf::Out<s64> out)  {
        auto read_lock = this->AcquireCacheInvalidationReadLock();
        return this->base_storage->GetSize(out.GetPointer());
    }

    Result StorageInterfaceAdapter::OperateRange(ams::sf::Out<fs::StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
        /* N includes this redundant check, so we will too. */
        R_UNLESS(out.GetPointer() != nullptr, fs::ResultNullptrArgument());

        out->Clear();
        if (op_id == static_cast<s32>(fs::OperationId::QueryRange)) {
            auto read_lock = this->AcquireCacheInvalidationReadLock();

            fs::StorageQueryRangeInfo info;
            R_TRY(this->base_storage->OperateRange(&info, sizeof(info), fs::OperationId::QueryRange, offset, size, nullptr, 0));
            out->Merge(info);
        }

        return ResultSuccess();
    }

}
