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
#include "fsa/fs_mount_utils.hpp"

namespace ams::fs {

    Result QueryMountSystemDataCacheSize(size_t *out, ncm::SystemDataId data_id) {
        return impl::QueryMountDataCacheSize(out, data_id, ncm::StorageId::BuiltInSystem);
    }

    Result MountSystemData(const char *name, ncm::SystemDataId data_id) {
        return impl::MountData(name, data_id, ncm::StorageId::BuiltInSystem);
    }

    Result MountSystemData(const char *name, ncm::SystemDataId data_id, void *cache_buffer, size_t cache_size) {
        return impl::MountData(name, data_id, ncm::StorageId::BuiltInSystem, cache_buffer, cache_size);
    }

}
