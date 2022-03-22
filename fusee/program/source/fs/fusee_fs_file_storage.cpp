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
#include <exosphere.hpp>
#include "fusee_fs_storage.hpp"

namespace ams::fs {

    Result FileHandleStorage::UpdateSize() {
        R_SUCCEED_IF(m_size != InvalidSize);
        return GetFileSize(std::addressof(m_size), m_handle);
    }

    Result FileHandleStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Immediately succeed if there's nothing to read. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, m_size), fs::ResultOutOfRange());

        return ReadFile(m_handle, offset, buffer, size, fs::ReadOption());
    }

    Result FileHandleStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Immediately succeed if there's nothing to write. */
        R_SUCCEED_IF(size == 0);

        /* Validate buffer. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Ensure our size is valid. */
        R_TRY(this->UpdateSize());

        /* Ensure our access is valid. */
        R_UNLESS(IStorage::CheckAccessRange(offset, size, m_size), fs::ResultOutOfRange());

        return WriteFile(m_handle, offset, buffer, size, fs::WriteOption());
    }

    Result FileHandleStorage::Flush() {
        return FlushFile(m_handle);
    }

    Result FileHandleStorage::GetSize(s64 *out_size) {
        R_TRY(this->UpdateSize());
        *out_size = m_size;
        return ResultSuccess();
    }

    Result FileHandleStorage::SetSize(s64 size) {
        m_size = InvalidSize;
        return SetFileSize(m_handle, size);
    }

}
