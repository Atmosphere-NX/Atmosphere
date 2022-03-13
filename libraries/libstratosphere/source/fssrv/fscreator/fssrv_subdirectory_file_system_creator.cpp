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

namespace ams::fssrv::fscreator {

    Result SubDirectoryFileSystemCreator::Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::fsa::IFileSystem> base_fs, const fs::Path &path) {
        /* Verify that we can the directory on the base filesystem. */
        {
            std::unique_ptr<fs::fsa::IDirectory> sub_dir;
            R_TRY(base_fs->OpenDirectory(std::addressof(sub_dir), path, fs::OpenDirectoryMode_Directory));
        }

        /* Allocate a SubDirectoryFileSystem. */
        auto sub_dir_fs = fs::AllocateShared<fssystem::SubDirectoryFileSystem>(std::move(base_fs));
        R_UNLESS(sub_dir_fs != nullptr, fs::ResultAllocationMemoryFailedInSubDirectoryFileSystemCreatorA());

        /* Initialize the new filesystem. */
        R_TRY(sub_dir_fs->Initialize(path));

        /* Return the new filesystem. */
        *out = std::move(sub_dir_fs);
        R_SUCCEED();
    }

}
