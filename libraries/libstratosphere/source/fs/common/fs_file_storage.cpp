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

namespace ams::fs {

    Result FileStorage::UpdateSize() {
        R_SUCCEED_IF(this->size != InvalidSize);
        return this->base_file->GetSize(std::addressof(this->size));
    }

    Result FileStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Immediately succeed if there's nothing to read. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());

        size_t read_size;
        return this->base_file->Read(std::addressof(read_size), offset, buffer, size);
    }

    Result FileStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Immediately succeed if there's nothing to write. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());

        return this->base_file->Write(offset, buffer, size, fs::WriteOption());
    }

    Result FileStorage::Flush() {
        return this->base_file->Flush();
    }

    Result FileStorage::GetSize(s64 *out_size) {
        R_TRY(this->UpdateSize());
        *out_size = this->size;
        return ResultSuccess();
    }

    Result FileStorage::SetSize(s64 size) {
        this->size = InvalidSize;
        return this->base_file->SetSize(size);
    }

    Result FileStorage::OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case OperationId::InvalidateCache:
            case OperationId::QueryRange:
                if (size == 0) {
                    if (op_id == OperationId::QueryRange) {
                        R_UNLESS(dst != nullptr,                     fs::ResultNullptrArgument());
                        R_UNLESS(dst_size == sizeof(QueryRangeInfo), fs::ResultInvalidSize());
                        reinterpret_cast<QueryRangeInfo *>(dst)->Clear();
                    }
                    return ResultSuccess();
                }
                R_TRY(this->UpdateSize());
                R_UNLESS(IStorage::CheckOffsetAndSize(offset, size), fs::ResultOutOfRange());
                return this->base_file->OperateRange(dst, dst_size, op_id, offset, size, src, src_size);
            default:
                return fs::ResultUnsupportedOperationInFileStorageA();
        }
    }

    Result FileStorageBasedFileSystem::Initialize(std::shared_ptr<fs::fsa::IFileSystem> base_file_system, const char *path, fs::OpenMode mode) {
        /* Open the file. */
        std::unique_ptr<fs::fsa::IFile> base_file;
        R_TRY(base_file_system->OpenFile(std::addressof(base_file), path, mode));

        /* Set the file. */
        this->SetFile(std::move(base_file));
        this->base_file_system = std::move(base_file_system);

        return ResultSuccess();
    }

    Result FileHandleStorage::UpdateSize() {
        R_SUCCEED_IF(this->size != InvalidSize);
        return GetFileSize(std::addressof(this->size), this->handle);
    }

    Result FileHandleStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Lock the mutex. */
        std::scoped_lock lk(this->mutex);

        /* Immediately succeed if there's nothing to read. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());

        return ReadFile(this->handle, offset, buffer, size, fs::ReadOption());
    }

    Result FileHandleStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Lock the mutex. */
        std::scoped_lock lk(this->mutex);

        /* Immediately succeed if there's nothing to write. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());

        return WriteFile(this->handle, offset, buffer, size, fs::WriteOption());
    }

    Result FileHandleStorage::Flush() {
        return FlushFile(this->handle);
    }

    Result FileHandleStorage::GetSize(s64 *out_size) {
        R_TRY(this->UpdateSize());
        *out_size = this->size;
        return ResultSuccess();
    }

    Result FileHandleStorage::SetSize(s64 size) {
        this->size = InvalidSize;
        return SetFileSize(this->handle, size);
    }

    Result FileHandleStorage::OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case OperationId::QueryRange:
                /* Validate buffer and size. */
                R_UNLESS(dst != nullptr,                     fs::ResultNullptrArgument());
                R_UNLESS(dst_size == sizeof(QueryRangeInfo), fs::ResultInvalidSize());

                return QueryRange(static_cast<QueryRangeInfo *>(dst), this->handle, offset, size);
            default:
                return fs::ResultUnsupportedOperationInFileStorageB();
        }
    }

}
