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
#pragma once
#include "fs_common.hpp"

namespace ams::fs {

    struct ReadOption {
        u32 value;

        static const ReadOption None;
    };

    inline constexpr const ReadOption ReadOption::None = {FsReadOption_None};

    inline constexpr bool operator==(const ReadOption &lhs, const ReadOption &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const ReadOption &lhs, const ReadOption &rhs) {
        return !(lhs == rhs);
    }

    static_assert(util::is_pod<ReadOption>::value && sizeof(ReadOption) == sizeof(u32));

    struct WriteOption {
        u32 value;

        constexpr inline bool HasFlushFlag() const {
            return this->value & FsWriteOption_Flush;
        }

        static const WriteOption None;
        static const WriteOption Flush;
    };

    inline constexpr const WriteOption WriteOption::None = {FsWriteOption_None};
    inline constexpr const WriteOption WriteOption::Flush = {FsWriteOption_Flush};

    inline constexpr bool operator==(const WriteOption &lhs, const WriteOption &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const WriteOption &lhs, const WriteOption &rhs) {
        return !(lhs == rhs);
    }

    static_assert(util::is_pod<WriteOption>::value && sizeof(WriteOption) == sizeof(u32));

    struct FileHandle {
        void *handle;
    };

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option);
    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size);
    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option);
    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size);
    Result GetFileSize(s64 *out, FileHandle handle);
    Result FlushFile(FileHandle handle);
    Result WriteFile(FileHandle handle, s64 offset, const void *buffer, size_t size, const fs::WriteOption &option);
    Result SetFileSize(FileHandle handle, s64 size);
    int GetFileOpenMode(FileHandle handle);
    void CloseFile(FileHandle handle);

}
