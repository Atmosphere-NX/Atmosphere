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

namespace ams::fs {

    Result FileStorage::UpdateSize() {
        R_SUCCEED_IF(m_size != InvalidSize);
        R_RETURN(m_base_file->GetSize(std::addressof(m_size)));
    }

    Result FileStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Immediately succeed if there's nothing to read. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_TRY(IStorage::CheckAccessRange(offset, size, m_size));

        size_t read_size;
        R_RETURN(m_base_file->Read(std::addressof(read_size), offset, buffer, size));
    }

    Result FileStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Immediately succeed if there's nothing to write. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_TRY(IStorage::CheckAccessRange(offset, size, m_size));

        R_RETURN(m_base_file->Write(offset, buffer, size, fs::WriteOption()));
    }

    Result FileStorage::Flush() {
        R_RETURN(m_base_file->Flush());
    }

    Result FileStorage::GetSize(s64 *out_size) {
        R_TRY(this->UpdateSize());
        *out_size = m_size;
        R_SUCCEED();
    }

    Result FileStorage::SetSize(s64 size) {
        m_size = InvalidSize;
        R_RETURN(m_base_file->SetSize(size));
    }

    Result FileStorage::OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case OperationId::Invalidate:
                R_RETURN(m_base_file->OperateRange(OperationId::Invalidate, offset, size));
            case OperationId::QueryRange:
                if (size == 0) {
                    R_UNLESS(dst != nullptr,                     fs::ResultNullptrArgument());
                    R_UNLESS(dst_size == sizeof(QueryRangeInfo), fs::ResultInvalidSize());
                    reinterpret_cast<QueryRangeInfo *>(dst)->Clear();
                    R_SUCCEED();
                }

                R_TRY(this->UpdateSize());
                R_TRY(IStorage::CheckOffsetAndSize(offset, size));

                R_RETURN(m_base_file->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
            default:
                R_THROW(fs::ResultUnsupportedOperateRangeForFileStorage());
        }
    }

    Result FileStorageBasedFileSystem::Initialize(std::shared_ptr<fs::fsa::IFileSystem> base_file_system, const fs::Path &path, fs::OpenMode mode) {
        /* Open the file. */
        std::unique_ptr<fs::fsa::IFile> base_file;
        R_TRY(base_file_system->OpenFile(std::addressof(base_file), path, mode));

        /* Set the file. */
        this->SetFile(std::move(base_file));
        m_base_file_system = std::move(base_file_system);

        R_SUCCEED();
    }

    Result FileHandleStorage::UpdateSize() {
        R_SUCCEED_IF(m_size != InvalidSize);
        R_RETURN(GetFileSize(std::addressof(m_size), m_handle));
    }

    Result FileHandleStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Lock the mutex. */
        std::scoped_lock lk(m_mutex);

        /* Immediately succeed if there's nothing to read. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_TRY(IStorage::CheckAccessRange(offset, size, m_size));

        R_RETURN(ReadFile(m_handle, offset, buffer, size, fs::ReadOption()));
    }

    Result FileHandleStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Lock the mutex. */
        std::scoped_lock lk(m_mutex);

        /* Immediately succeed if there's nothing to write. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_TRY(IStorage::CheckAccessRange(offset, size, m_size));

        R_RETURN(WriteFile(m_handle, offset, buffer, size, fs::WriteOption()));
    }

    Result FileHandleStorage::Flush() {
        R_RETURN(FlushFile(m_handle));
    }

    Result FileHandleStorage::GetSize(s64 *out_size) {
        R_TRY(this->UpdateSize());
        *out_size = m_size;
        R_SUCCEED();
    }

    Result FileHandleStorage::SetSize(s64 size) {
        m_size = InvalidSize;
        R_RETURN(SetFileSize(m_handle, size));
    }

    Result FileHandleStorage::OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        AMS_UNUSED(src, src_size);

        switch (op_id) {
            case OperationId::QueryRange:
                /* Validate buffer and size. */
                R_UNLESS(dst != nullptr,                     fs::ResultNullptrArgument());
                R_UNLESS(dst_size == sizeof(QueryRangeInfo), fs::ResultInvalidSize());

                R_RETURN(QueryRange(static_cast<QueryRangeInfo *>(dst), m_handle, offset, size));
            default:
                R_THROW(fs::ResultUnsupportedOperateRangeForFileHandleStorage());
        }
    }

}
