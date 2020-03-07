/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include "ncm_fs_utils.hpp"

namespace ams::ncm::impl {

    namespace {

        Result EnsureDirectory(const char *path) {
            /* Create the path, and allow it to already exist. */
            R_TRY_CATCH(fs::CreateDirectory(path)) {
                R_CONVERT(fs::ResultPathAlreadyExists, ResultSuccess())
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

        Result EnsureDirectoryRecursivelyImpl(const char *path, bool create_last) {
            char work_buf[fs::EntryNameLengthMax];

            /* Ensure the path is not too long. */
            const size_t len = std::strlen(path);
            R_UNLESS(len + 1 <= sizeof(work_buf), ResultAllocationFailed());

            /* Copy in the path. */
            std::strncpy(work_buf, path, sizeof(work_buf));

            /* Create all but the last directory. */
            for (size_t i = 0; i < len; i++) {
                if (i > 0 && fs::PathTool::IsSeparator(work_buf[i]) && !fs::PathTool::IsDriveSeparator(work_buf[i-1])) {
                    work_buf[i] = fs::StringTraits::NullTerminator;
                    R_TRY(EnsureDirectory(work_buf));
                    work_buf[i] = fs::StringTraits::DirectorySeparator;
                }
            }

            /* Create the last directory if requested. */
            if (create_last) {
                R_TRY(EnsureDirectory(path));
            }

            return ResultSuccess();
        }

        Result HasEntry(bool *out, const char *path, fs::DirectoryEntryType type) {
            /* Set out to false initially. */
            *out = false;

            /* Try to get the entry type. */
            fs::DirectoryEntryType entry_type;
            R_TRY_CATCH(fs::GetEntryType(std::addressof(entry_type), path)) {
                /* If the path doesn't exist, nothing has gone wrong. */
                R_CONVERT(fs::ResultPathNotFound, ResultSuccess());
            } R_END_TRY_CATCH;

            /* We succeeded. */
            *out = entry_type == type;
            return ResultSuccess();
        }

        std::atomic<u32> g_mount_name_count;

    }

    Result HasFile(bool *out, const char *path) {
        return HasEntry(out, path, fs::DirectoryEntryType_File);
    }

    Result HasDirectory(bool *out, const char *path) {
        return HasEntry(out, path, fs::DirectoryEntryType_Directory);
    }

    Result EnsureDirectoryRecursively(const char *path) {
        return EnsureDirectoryRecursivelyImpl(path, true);
    }

    Result EnsureParentDirectoryRecursively(const char *path) {
        return EnsureDirectoryRecursivelyImpl(path, false);
    }

    bool PathView::HasPrefix(std::string_view prefix) const {
        return this->path.compare(0, prefix.length(), prefix) == 0;
    }

    bool PathView::HasSuffix(std::string_view suffix) const {
        return this->path.compare(this->path.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    std::string_view PathView::GetFileName() const {
        auto pos = this->path.find_last_of("/");
        return pos != std::string_view::npos ? this->path.substr(pos + 1) : this->path;
    }

    MountName CreateUniqueMountName() {
        MountName name = {};
        std::snprintf(name.str, sizeof(name.str), "@ncm%08x", g_mount_name_count.fetch_add(1));
        return name;
    }

    RootDirectoryPath GetRootDirectoryPath(const MountName &mount_name) {
        RootDirectoryPath path = {};
        std::snprintf(path.str, sizeof(path.str), "%s:/", mount_name.str);

        return path;
    }

    Result CopyFile(const char *dst_path, const char *src_path) {
        fs::FileHandle src_file, dst_file;

        /* Open the source file and get its size. */
        R_TRY(fs::OpenFile(std::addressof(src_file), src_path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(src_file); };

        s64 file_size;
        R_TRY(fs::GetFileSize(std::addressof(file_size), src_file));

        /* Create the destination file. */
        R_TRY(fs::CreateFile(dst_path, file_size));

        /* Open the destination file. */
        R_TRY(fs::OpenFile(std::addressof(dst_file), dst_path, fs::OpenMode_Write));
        ON_SCOPE_EXIT { fs::CloseFile(dst_file); };

        /* Allocate a buffer with which to copy. */
        constexpr size_t BufferSize = 4_KB;
        AutoBuffer buffer;
        R_TRY(buffer.Initialize(BufferSize));

        /* Repeatedly read until we've copied all the data. */
        s64 offset = 0;
        while (offset < file_size) {
            const size_t read_size = std::min(static_cast<size_t>(file_size - offset), buffer.GetSize());
            R_TRY(fs::ReadFile(src_file, offset, buffer.Get(), read_size));
            R_TRY(fs::WriteFile(dst_file, offset, buffer.Get(), read_size, fs::WriteOption::None));
            offset += read_size;
        }

        /* Flush the destination file. */
        R_TRY(fs::FlushFile(dst_file));

        return ResultSuccess();
    }

}
