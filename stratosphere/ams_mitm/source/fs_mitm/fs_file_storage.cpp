/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <cstring>
#include <switch.h>
#include <stratosphere.hpp>

#include "../utils.hpp"
#include "fs_file_storage.hpp"

Result FileStorage::UpdateSize() {
    if (this->size == InvalidSize) {
        return this->file->GetSize(&this->size);
    }
    return ResultSuccess();
}

Result FileStorage::Read(void *buffer, size_t size, u64 offset) {
    u64 read_size;

    if (size == 0) {
        return ResultSuccess();
    }
    if (buffer == nullptr) {
        return ResultFsNullptrArgument;
    }
    R_TRY(this->UpdateSize());
    if (!IStorage::IsRangeValid(offset, size, this->size)) {
        return ResultFsOutOfRange;
    }

    /* Nintendo doesn't do check output read size, but we will for safety. */
    R_TRY(this->file->Read(&read_size, offset, buffer, size));
    if (read_size != size && read_size) {
        return this->Read(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + read_size), size - read_size, offset + read_size);
    }
    return ResultSuccess();
}

Result FileStorage::Write(void *buffer, size_t size, u64 offset) {
    if (size == 0) {
        return ResultSuccess();
    }
    if (buffer == nullptr) {
        return ResultFsNullptrArgument;
    }
    R_TRY(this->UpdateSize());
    if (!IStorage::IsRangeValid(offset, size, this->size)) {
        return ResultFsOutOfRange;
    }

    return this->file->Write(offset, buffer, size);
}

Result FileStorage::Flush() {
    return this->file->Flush();
}

Result FileStorage::GetSize(u64 *out_size) {
    R_TRY(this->UpdateSize());
    *out_size = this->size;
    return ResultSuccess();
}

Result FileStorage::SetSize(u64 size) {
    this->size = InvalidSize;
    return this->file->SetSize(size);
}

Result FileStorage::OperateRange(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) {
    switch (operation_type) {
        case FsOperationId_InvalidateCache:
        case FsOperationId_QueryRange:
            if (size == 0) {
                if (operation_type == FsOperationId_QueryRange) {
                    if (out_range_info == nullptr) {
                        return ResultFsNullptrArgument;
                    }
                    /* N checks for size == sizeof(*out_range_info) here, but that's because their wrapper api is bad. */
                    std::memset(out_range_info, 0, sizeof(*out_range_info));
                }
                return ResultSuccess();
            }
            R_TRY(this->UpdateSize());
            /* N checks for positivity + signed overflow on offset/size here, but we're using unsigned types... */
            return this->file->OperateRange(operation_type, offset, size, out_range_info);
        default:
            return ResultFsUnsupportedOperation;
    }
}