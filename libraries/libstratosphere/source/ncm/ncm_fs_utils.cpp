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
#include "ncm_fs_utils.hpp"

namespace ams::ncm::impl {

    namespace {

        constinit std::atomic<u32> g_mount_name_count;

    }

    bool PathView::HasPrefix(util::string_view prefix) const {
        return m_path.compare(0, prefix.length(), prefix) == 0;
    }

    bool PathView::HasSuffix(util::string_view suffix) const {
        return m_path.compare(m_path.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    util::string_view PathView::GetFileName() const {
        auto pos = m_path.find_last_of("/");
        return pos != util::string_view::npos ? m_path.substr(pos + 1) : m_path;
    }

    MountName CreateUniqueMountName() {
        MountName name = {};
        util::SNPrintf(name.str, sizeof(name.str), "@ncm%08x", g_mount_name_count.fetch_add(1));
        return name;
    }

    RootDirectoryPath GetRootDirectoryPath(const MountName &mount_name) {
        RootDirectoryPath path = {};
        util::SNPrintf(path.str, sizeof(path.str), "%s:/", mount_name.str);

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
