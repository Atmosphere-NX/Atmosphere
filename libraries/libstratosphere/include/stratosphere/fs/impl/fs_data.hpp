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

namespace ams::fs::impl {

    Result QueryMountDataCacheSize(size_t *out, ncm::DataId data_id, ncm::StorageId storage_id);

    Result MountData(const char *name, ncm::DataId data_id, ncm::StorageId storage_id);
    Result MountData(const char *name, ncm::DataId data_id, ncm::StorageId storage_id, void *cache_buffer, size_t cache_size);
    Result MountData(const char *name, ncm::DataId data_id, ncm::StorageId storage_id, void *cache_buffer, size_t cache_size, bool use_data_cache, bool use_path_cache);

}
