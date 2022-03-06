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

namespace ams::fs {

    Result QueryMountSystemDataCacheSize(size_t *out, ncm::SystemDataId data_id) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM(impl::QueryMountDataCacheSize(out, data_id, ncm::StorageId::BuiltInSystem), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_QUERY_MOUNT_SYSTEM_DATA_CACHE_SIZE(data_id, out)));
        R_SUCCEED();
    }

    Result MountSystemData(const char *name, ncm::SystemDataId data_id) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(impl::MountData(name, data_id, ncm::StorageId::BuiltInSystem), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_SYSTEM_DATA(name, data_id)));
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);
        R_SUCCEED();
    }

    Result MountSystemData(const char *name, ncm::SystemDataId data_id, void *cache_buffer, size_t cache_size) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(impl::MountData(name, data_id, ncm::StorageId::BuiltInSystem, cache_buffer, cache_size), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_SYSTEM_DATA(name, data_id)));
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);
        R_SUCCEED();
    }

}
