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
#include "htcfs_file_service_object.hpp"

namespace ams::htcfs {

    FileServiceObject::FileServiceObject(s32 handle) : m_handle(handle) { /* ... */ }

    FileServiceObject::~FileServiceObject() {
        /* TODO */
        AMS_ABORT("htcfs::GetClient().CloseFile(m_handle);");
    }

    Result FileServiceObject::ReadFile(ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, ams::fs::ReadOption option) {
        AMS_ABORT("FileServiceObject::ReadFile");
    }

    Result FileServiceObject::WriteFile(s64 offset, const ams::sf::InNonSecureBuffer &buffer, ams::fs::WriteOption option) {
        AMS_ABORT("FileServiceObject::WriteFile");
    }

    Result FileServiceObject::GetFileSize(ams::sf::Out<s64> out) {
        AMS_ABORT("FileServiceObject::GetFileSize");
    }

    Result FileServiceObject::SetFileSize(s64 size) {
        AMS_ABORT("FileServiceObject::SetFileSize");
    }

    Result FileServiceObject::FlushFile() {
        AMS_ABORT("FileServiceObject::FlushFile");
    }

    Result FileServiceObject::SetPriorityForFile(s32 priority) {
        AMS_ABORT("FileServiceObject::SetPriorityForFile");
    }

    Result FileServiceObject::GetPriorityForFile(ams::sf::Out<s32> out) {
        AMS_ABORT("FileServiceObject::GetPriorityForFile");
    }

}
