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
#include "fs_directory_accessor.hpp"
#include "fs_filesystem_accessor.hpp"

namespace ams::fs {

    namespace {

        ALWAYS_INLINE impl::DirectoryAccessor *Get(DirectoryHandle handle) {
            return reinterpret_cast<impl::DirectoryAccessor *>(handle.handle);
        }

    }

    Result ReadDirectory(s64 *out_count, DirectoryEntry *out_entries, DirectoryHandle handle, s64 max_entries) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG(Get(handle)->Read(out_count, out_entries, max_entries), handle, AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_DIRECTORY(out_count, max_entries)));
        return ResultSuccess();
    }

    Result GetDirectoryEntryCount(s64 *out, DirectoryHandle handle) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG(Get(handle)->GetEntryCount(out), handle, AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_DIRECTORY_ENTRY_COUNT(out)));
        return ResultSuccess();
    }

    void CloseDirectory(DirectoryHandle handle) {
        AMS_FS_IMPL_ACCESS_LOG((delete Get(handle), ResultSuccess()), handle, AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE);
    }

}
