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
        return Get(handle)->Read(out_count, out_entries, max_entries);
    }

    Result GetDirectoryEntryCount(s64 *out, DirectoryHandle handle) {
        return Get(handle)->GetEntryCount(out);
    }

    void CloseDirectory(DirectoryHandle handle) {
        delete Get(handle);
    }

}
