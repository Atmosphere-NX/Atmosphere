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
#include "sprofile_srv_profile_manager.hpp"

namespace ams::sprofile::srv {

    Result ReadFile(const char *path, void *dst, size_t size, s64 offset) {
        /* Open the file. */
        fs::FileHandle file;
        R_TRY_CATCH(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read)) {
            R_CATCH_RETHROW(fs::ResultPathNotFound) /* It's okay if the file doesn't exist. */
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Read the file. */
        size_t read_size;
        R_TRY(fs::ReadFile(std::addressof(read_size), file, offset, dst, size));

        /* Check the size was correct. */
        AMS_ABORT_UNLESS(size == read_size);

        return ResultSuccess();
    }

    Result WriteFile(const char *path, const void *src, size_t size) {
        /* Create the file. */
        R_TRY_CATCH(fs::CreateFile(path, size)) {
            R_CATCH(fs::ResultPathAlreadyExists) { /* It's okay if the file already exists. */ }
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        /* Open the file. */
        fs::FileHandle file;
        R_ABORT_UNLESS(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Set the file size. */
        R_ABORT_UNLESS(fs::SetFileSize(file, size));

        /* Write the file. */
        return fs::WriteFile(file, 0, src, size, fs::WriteOption::Flush);
    }

    Result MoveFile(const char *src_path, const char *dst_path) {
        /* Require that the source path is a file. */
        {
            fs::DirectoryEntryType type;
            R_ABORT_UNLESS(fs::GetEntryType(std::addressof(type), src_path));
            AMS_ABORT_UNLESS(type == fs::DirectoryEntryType_File);
        }

        /* Delete the destination file. */
        R_TRY_CATCH(fs::DeleteFile(dst_path)) {
            R_CATCH(fs::ResultPathNotFound) { /* It's okay if the dst path doesn't exist. */ }
        } R_END_TRY_CATCH;

        /* Move the source file to the destination file. */
        R_TRY(fs::RenameFile(src_path, dst_path));

        return ResultSuccess();
    }

    Result EnsureDirectory(const char *path) {
        R_TRY_CATCH(fs::CreateDirectory(path)) {
            R_CATCH(fs::ResultPathAlreadyExists) { /* It's okay if the directory already exists. */ }
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

}
