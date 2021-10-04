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
#include "htcfs_file_service_object.hpp"
#include "htcfs_client.hpp"

namespace ams::htcfs {

    FileServiceObject::FileServiceObject(s32 handle) : m_handle(handle) { /* ... */ }

    FileServiceObject::~FileServiceObject() {
        htcfs::GetClient().CloseFile(m_handle);
    }

    Result FileServiceObject::ReadFile(ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, ams::fs::ReadOption option) {
        /* Validate offset. */
        R_UNLESS(offset >= 0, htcfs::ResultInvalidArgument());

        if (buffer.GetSize() >= ClientImpl::MaxPacketBodySize) {
            return htcfs::GetClient().ReadFileLarge(out.GetPointer(), buffer.GetPointer(), m_handle, offset, buffer.GetSize(), option);
        } else {
            return htcfs::GetClient().ReadFile(out.GetPointer(), buffer.GetPointer(), m_handle, offset, buffer.GetSize(), option);
        }
    }

    Result FileServiceObject::WriteFile(s64 offset, const ams::sf::InNonSecureBuffer &buffer, ams::fs::WriteOption option) {
        /* Validate offset. */
        R_UNLESS(offset >= 0, htcfs::ResultInvalidArgument());

        if (buffer.GetSize() >= ClientImpl::MaxPacketBodySize) {
            return htcfs::GetClient().WriteFileLarge(buffer.GetPointer(), m_handle, offset, buffer.GetSize(), option);
        } else {
            return htcfs::GetClient().WriteFile(buffer.GetPointer(), m_handle, offset, buffer.GetSize(), option);
        }
    }

    Result FileServiceObject::GetFileSize(ams::sf::Out<s64> out) {
        return htcfs::GetClient().GetFileSize(out.GetPointer(), m_handle);
    }

    Result FileServiceObject::SetFileSize(s64 size) {
        /* Validate size. */
        R_UNLESS(size >= 0, htcfs::ResultInvalidArgument());

        return htcfs::GetClient().SetFileSize(size, m_handle);
    }

    Result FileServiceObject::FlushFile() {
        return htcfs::GetClient().FlushFile(m_handle);
    }

    Result FileServiceObject::SetPriorityForFile(s32 priority) {
        return htcfs::GetClient().SetPriorityForFile(priority, m_handle);
    }

    Result FileServiceObject::GetPriorityForFile(ams::sf::Out<s32> out) {
        return htcfs::GetClient().GetPriorityForFile(out.GetPointer(), m_handle);
    }

}
