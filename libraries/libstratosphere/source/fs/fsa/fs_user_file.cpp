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
#include "fs_file_accessor.hpp"
#include "fs_filesystem_accessor.hpp"

namespace ams::fs {

    namespace {

        ALWAYS_INLINE impl::FileAccessor *Get(FileHandle handle) {
            return reinterpret_cast<impl::FileAccessor *>(handle.handle);
        }

    }

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
        size_t read_size;
        R_TRY(ReadFile(std::addressof(read_size), handle, offset, buffer, size, option));
        R_UNLESS(read_size == size, fs::ResultOutOfRange());
        return ResultSuccess();
    }

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size) {
        return ReadFile(handle, offset, buffer, size, ReadOption());
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
        return Get(handle)->Read(out, offset, buffer, size, option);
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size) {
        return ReadFile(out, handle, offset, buffer, size, ReadOption());
    }

    Result GetFileSize(s64 *out, FileHandle handle) {
        return Get(handle)->GetSize(out);
    }

    Result FlushFile(FileHandle handle) {
        return Get(handle)->Flush();
    }

    Result WriteFile(FileHandle handle, s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) {
        return Get(handle)->Write(offset, buffer, size, option);
    }

    Result SetFileSize(FileHandle handle, s64 size) {
        return Get(handle)->SetSize(size);
    }

    int GetFileOpenMode(FileHandle handle) {
        return Get(handle)->GetOpenMode();
    }

    void CloseFile(FileHandle handle) {
        delete Get(handle);
    }

    Result QueryRange(QueryRangeInfo *out, FileHandle handle, s64 offset, s64 size) {
        return Get(handle)->OperateRange(out, sizeof(*out), OperationId::QueryRange, offset, size, nullptr, 0);
    }

}
