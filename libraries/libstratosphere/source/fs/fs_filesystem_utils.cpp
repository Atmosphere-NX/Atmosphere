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
#include "fsa/fs_filesystem_accessor.hpp"

namespace ams::fs {

    Result EnsureDirectoryRecursively(const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Use the system implementation. */
        return fssystem::EnsureDirectoryRecursively(accessor->GetRawFileSystemUnsafe(), sub_path);
    }

    Result EnsureParentDirectoryRecursively(const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Use the system implementation. */
        return fssystem::EnsureParentDirectoryRecursively(accessor->GetRawFileSystemUnsafe(), sub_path);
    }

    Result HasFile(bool *out, const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return fssystem::HasFile(out, accessor->GetRawFileSystemUnsafe(), sub_path);
    }

    Result HasDirectory(bool *out, const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return fssystem::HasDirectory(out, accessor->GetRawFileSystemUnsafe(), sub_path);
    }

}
