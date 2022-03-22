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
#include "htcfs_directory_service_object.hpp"
#include "htcfs_client.hpp"

namespace ams::htcfs {

    DirectoryServiceObject::DirectoryServiceObject(s32 handle) : m_handle(handle) { /* ... */ }

    DirectoryServiceObject::~DirectoryServiceObject() {
        htcfs::GetClient().CloseDirectory(m_handle);
    }

    Result DirectoryServiceObject::GetEntryCount(ams::sf::Out<s64> out) {
        return htcfs::GetClient().GetEntryCount(out.GetPointer(), m_handle);
    }

    Result DirectoryServiceObject::Read(ams::sf::Out<s64> out, const ams::sf::OutMapAliasArray<fs::DirectoryEntry> &out_entries) {
        if (out_entries.GetSize() * sizeof(fs::DirectoryEntry) >= ClientImpl::MaxPacketBodySize) {
            return htcfs::GetClient().ReadDirectoryLarge(out.GetPointer(), out_entries.GetPointer(), out_entries.GetSize(), m_handle);
        } else {
            return htcfs::GetClient().ReadDirectory(out.GetPointer(), out_entries.GetPointer(), out_entries.GetSize(), m_handle);
        }
    }

    Result DirectoryServiceObject::SetPriorityForDirectory(s32 priority) {
        return htcfs::GetClient().SetPriorityForDirectory(priority, m_handle);
    }

    Result DirectoryServiceObject::GetPriorityForDirectory(ams::sf::Out<s32> out) {
        return htcfs::GetClient().GetPriorityForDirectory(out.GetPointer(), m_handle);
    }


}
