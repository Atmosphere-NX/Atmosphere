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
        AMS_FS_R_TRY(ReadFile(std::addressof(read_size), handle, offset, buffer, size, option));
        AMS_FS_R_UNLESS(read_size == size, fs::ResultOutOfRange());
        return ResultSuccess();
    }

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size) {
        return ReadFile(handle, offset, buffer, size, ReadOption());
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
        AMS_FS_R_TRY(Get(handle)->Read(out, offset, buffer, size, option));
        return ResultSuccess();
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size) {
        return ReadFile(out, handle, offset, buffer, size, ReadOption());
    }

    Result GetFileSize(s64 *out, FileHandle handle) {
        AMS_FS_R_TRY(Get(handle)->GetSize(out));
        return ResultSuccess();
    }

    Result FlushFile(FileHandle handle) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG(Get(handle)->Flush(), handle, AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE));
        return ResultSuccess();
    }

    Result WriteFile(FileHandle handle, s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG(Get(handle)->Write(offset, buffer, size, option), handle, AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE(option), offset, size));
        return ResultSuccess();
    }

    Result SetFileSize(FileHandle handle, s64 size) {
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG(Get(handle)->SetSize(size), handle, AMS_FS_IMPL_ACCESS_LOG_FORMAT_SIZE, size));
        return ResultSuccess();
    }

    int GetFileOpenMode(FileHandle handle) {
        return Get(handle)->GetOpenMode();
    }

    void CloseFile(FileHandle handle) {
        AMS_FS_IMPL_ACCESS_LOG((delete Get(handle), ResultSuccess()), handle, AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE);
    }

    Result QueryRange(QueryRangeInfo *out, FileHandle handle, s64 offset, s64 size) {
        AMS_FS_R_TRY(Get(handle)->OperateRange(out, sizeof(*out), OperationId::QueryRange, offset, size, nullptr, 0));
        return ResultSuccess();
    }

}
