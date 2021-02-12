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
#include "htcfs_file_system_service_object.hpp"

namespace ams::htcfs {

    Result FileSystemServiceObject::OpenFile(sf::Out<sf::SharedPointer<tma::IFileAccessor>> out, const tma::Path &path, u32 open_mode, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::OpenFile");
    }

    Result FileSystemServiceObject::FileExists(sf::Out<bool> out, const tma::Path &path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::FileExists");
    }

    Result FileSystemServiceObject::DeleteFile(const tma::Path &path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::DeleteFile");
    }

    Result FileSystemServiceObject::RenameFile(const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::RenameFile");
    }

    Result FileSystemServiceObject::GetIOType(sf::Out<s32> out, const tma::Path &path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::GetIOType");
    }

    Result FileSystemServiceObject::OpenDirectory(sf::Out<sf::SharedPointer<tma::IDirectoryAccessor>> out, const tma::Path &path, s32 open_mode, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::OpenDirectory");
    }

    Result FileSystemServiceObject::DirectoryExists(sf::Out<bool> out, const tma::Path &path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::DirectoryExists");
    }

    Result FileSystemServiceObject::CreateDirectory(const tma::Path &path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::CreateDirectory");
    }

    Result FileSystemServiceObject::DeleteDirectory(const tma::Path &path, bool recursively, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::DeleteDirectory");
    }

    Result FileSystemServiceObject::RenameDirectory(const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::RenameDirectory");
    }

    Result FileSystemServiceObject::CreateFile(const tma::Path &path, s64 size, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::CreateFile");
    }

    Result FileSystemServiceObject::GetFileTimeStamp(sf::Out<u64> out_create, sf::Out<u64> out_access, sf::Out<u64> out_modify, const tma::Path &path, bool case_sensitive) {
        AMS_ABORT("FileSystemServiceObject::GetFileTimeStamp");
    }

    Result FileSystemServiceObject::GetCaseSensitivePath(const tma::Path &path, const sf::OutBuffer &out) {
        AMS_ABORT("FileSystemServiceObject::GetCaseSensitivePath");
    }

    Result FileSystemServiceObject::GetDiskFreeSpaceExW(sf::Out<s64> out_free, sf::Out<s64> out_total, sf::Out<s64> out_total_free, const tma::Path &path) {
        AMS_ABORT("FileSystemServiceObject::GetDiskFreeSpaceExW");
    }

}
