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
#include <stratosphere.hpp>
#include "fsa/fs_mount_utils.hpp"

namespace ams::fs::impl {

    namespace {

        Result OpenDataStorageByDataId(std::unique_ptr<ams::fs::IStorage> *out, ncm::DataId data_id, ncm::StorageId storage_id) {
            /* Open storage using libnx bindings. */
            ::FsStorage s;
            R_TRY_CATCH(fsOpenDataStorageByDataId(std::addressof(s), data_id.value, static_cast<::NcmStorageId>(storage_id))) {
                R_CONVERT(ncm::ResultContentMetaNotFound, fs::ResultTargetNotFound());
            } R_END_TRY_CATCH;

            auto storage = std::make_unique<RemoteStorage>(s);
            R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInDataA());

            *out = std::move(storage);
            return ResultSuccess();
        }

        Result MountDataImpl(const char *name, ncm::DataId data_id, ncm::StorageId storage_id, void *cache_buffer, size_t cache_size, bool use_cache, bool use_data_cache, bool use_path_cache) {
            std::unique_ptr<fs::IStorage> storage;
            R_TRY(OpenDataStorageByDataId(std::addressof(storage), data_id, storage_id));

            auto fs = std::make_unique<RomFsFileSystem>();
            R_UNLESS(fs != nullptr, fs::ResultAllocationFailureInDataB());
            R_TRY(fs->Initialize(std::move(storage), cache_buffer, cache_size, use_cache));

            return fsa::Register(name, std::move(fs), nullptr, use_data_cache, use_path_cache, false);
        }

    }

    Result QueryMountDataCacheSize(size_t *out, ncm::DataId data_id, ncm::StorageId storage_id) {
        R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        std::unique_ptr<fs::IStorage> storage;
        R_TRY(OpenDataStorageByDataId(std::addressof(storage), data_id, storage_id));

        size_t size = 0;
        R_TRY(RomFsFileSystem::GetRequiredWorkingMemorySize(std::addressof(size), storage.get()));

        constexpr size_t MinimumCacheSize = 32;
        *out = std::max(size, MinimumCacheSize);
        return ResultSuccess();
    }

    Result MountData(const char *name, ncm::DataId data_id, ncm::StorageId storage_id) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        return MountDataImpl(name, data_id, storage_id, nullptr, 0, false, false, false);
    }

    Result MountData(const char *name, ncm::DataId data_id, ncm::StorageId storage_id, void *cache_buffer, size_t cache_size) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        R_UNLESS(cache_buffer != nullptr, fs::ResultNullptrArgument());

        return MountDataImpl(name, data_id, storage_id, cache_buffer, cache_size, true, false, false);
    }

    Result MountData(const char *name, ncm::DataId data_id, ncm::StorageId storage_id, void *cache_buffer, size_t cache_size, bool use_data_cache, bool use_path_cache) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        R_UNLESS(cache_buffer != nullptr, fs::ResultNullptrArgument());

        return MountDataImpl(name, data_id, storage_id, cache_buffer, cache_size, true, use_data_cache, use_path_cache);
    }

}
