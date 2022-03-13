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

    Result LocalFileSystemCreator::Create(std::shared_ptr<fs::fsa::IFileSystem> *out, const fs::Path &path, bool case_sensitive, bool ensure_root, Result on_path_not_found) {
        /* Check that we're configured for development. */
        R_UNLESS(m_is_development, fs::ResultPermissionDeniedForCreateHostFileSystem());

        /* Allocate a local filesystem. */
        auto local_fs = fs::AllocateShared<fssystem::LocalFileSystem>();
        R_UNLESS(local_fs != nullptr, fs::ResultAllocationMemoryFailedInLocalFileSystemCreatorA());

        /* If we're supposed to make sure the root path exists, do so. */
        if (ensure_root) {
            /* Sanity check that the path will be possible to create. */
            AMS_ASSERT(!path.IsEmpty());

            /* Initialize the local fs with an empty path. */
            fs::Path empty_path;
            R_TRY(empty_path.InitializeAsEmpty());
            R_TRY(local_fs->Initialize(empty_path, fssystem::PathCaseSensitiveMode_CaseInsensitive));

            /* Ensure the directory exists. */
            if (const Result ensure_result = fssystem::EnsureDirectory(local_fs.get(), path); R_FAILED(ensure_result)) {
                if (R_SUCCEEDED(on_path_not_found)) {
                    R_THROW(ensure_result);
                } else {
                    R_THROW(on_path_not_found);
                }
            }
        }

        /* Initialize the local filesystem. */
        R_TRY_CATCH(local_fs->Initialize(path, case_sensitive ? fssystem::PathCaseSensitiveMode_CaseSensitive : fssystem::PathCaseSensitiveMode_CaseInsensitive)) {
            R_CATCH(fs::ResultPathNotFound) {
                if (R_SUCCEEDED(on_path_not_found)) {
                    R_THROW(R_CURRENT_RESULT);
                } else {
                    R_THROW(on_path_not_found);
                }
            }
        } R_END_TRY_CATCH;

        /* Set the output fs. */
        *out = std::move(local_fs);
        R_SUCCEED();
    }

}
