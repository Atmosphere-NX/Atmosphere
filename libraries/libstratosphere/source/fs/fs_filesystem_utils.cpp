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
#include "fsa/fs_filesystem_accessor.hpp"

namespace ams::fs {

    Result EnsureDirectory(const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Set up the true sub path. */
        fs::Path sub_fs_path;
        R_TRY(accessor->SetUpPath(std::addressof(sub_fs_path), sub_path));

        /* Use the system implementation. */
        R_RETURN(fssystem::EnsureDirectory(accessor->GetRawFileSystemUnsafe(), sub_fs_path));
    }

    Result EnsureParentDirectory(const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Set up the true sub path. */
        fs::Path sub_fs_path;
        R_TRY(accessor->SetUpPath(std::addressof(sub_fs_path), sub_path));

        /* Convert the path to the parent directory path. */
        R_TRY(sub_fs_path.RemoveChild());

        /* Use the system implementation. */
        R_RETURN(fssystem::EnsureDirectory(accessor->GetRawFileSystemUnsafe(), sub_fs_path));
    }

    Result HasFile(bool *out, const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Set up the true sub path. */
        fs::Path sub_fs_path;
        R_TRY(accessor->SetUpPath(std::addressof(sub_fs_path), sub_path));

        R_RETURN(fssystem::HasFile(out, accessor->GetRawFileSystemUnsafe(), sub_fs_path));
    }

    Result HasDirectory(bool *out, const char *path) {
        /* Get the filesystem accessor and sub path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Set up the true sub path. */
        fs::Path sub_fs_path;
        R_TRY(accessor->SetUpPath(std::addressof(sub_fs_path), sub_path));

        R_RETURN(fssystem::HasDirectory(out, accessor->GetRawFileSystemUnsafe(), sub_fs_path));
    }

}
