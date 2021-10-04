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
#pragma once
#include <stratosphere.hpp>

namespace ams::htcfs {

    constexpr inline bool DefaultCaseSensitivityForDeprecatedFileSystemServiceObject = false;

    class FileSystemServiceObject {
        public:
            Result OpenFile(sf::Out<sf::SharedPointer<tma::IFileAccessor>> out, const tma::Path &path, u32 open_mode, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result FileExists(sf::Out<bool> out, const tma::Path &path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result DeleteFile(const tma::Path &path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result RenameFile(const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result GetIOType(sf::Out<s32> out, const tma::Path &path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result OpenDirectory(sf::Out<sf::SharedPointer<tma::IDirectoryAccessor>> out, const tma::Path &path, s32 open_mode, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result DirectoryExists(sf::Out<bool> out, const tma::Path &path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result CreateDirectory(const tma::Path &path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result DeleteDirectory(const tma::Path &path, bool recursively, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result RenameDirectory(const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result CreateFile(const tma::Path &path, s64 size, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result GetFileTimeStamp(sf::Out<u64> out_create, sf::Out<u64> out_access, sf::Out<u64> out_modify, const tma::Path &path, bool case_sensitive = DefaultCaseSensitivityForDeprecatedFileSystemServiceObject);
            Result GetCaseSensitivePath(const tma::Path &path, const sf::OutBuffer &out);
            Result GetDiskFreeSpaceExW(sf::Out<s64> out_free, sf::Out<s64> out_total, sf::Out<s64> out_total_free, const tma::Path &path);
    };
    static_assert(tma::IsIFileManager<FileSystemServiceObject>);
    static_assert(tma::IsIDeprecatedFileManager<FileSystemServiceObject>);

}
