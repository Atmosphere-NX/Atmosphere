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
#include <vapours.hpp>
#include <stratosphere/tma/tma_i_directory_accessor.hpp>
#include <stratosphere/tma/tma_i_file_accessor.hpp>
#include <stratosphere/tma/tma_path.hpp>

/* NOTE: Minimum firmware version not enforced for any commands. */
#define AMS_TMA_I_FILE_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                                                                 \
    AMS_SF_METHOD_INFO(C, H,  0, Result, OpenFile,             (sf::Out<sf::SharedPointer<tma::IFileAccessor>> out, const tma::Path &path, u32 open_mode, bool case_sensitive),         (out, path, open_mode, case_sensitive))                     \
    AMS_SF_METHOD_INFO(C, H,  1, Result, FileExists,           (sf::Out<bool> out, const tma::Path &path, bool case_sensitive),                                                         (out, path, case_sensitive))                                \
    AMS_SF_METHOD_INFO(C, H,  2, Result, DeleteFile,           (const tma::Path &path, bool case_sensitive),                                                                            (path, case_sensitive))                                     \
    AMS_SF_METHOD_INFO(C, H,  3, Result, RenameFile,           (const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive),                                             (old_path, new_path, case_sensitive))                       \
    AMS_SF_METHOD_INFO(C, H,  4, Result, GetIOType,            (sf::Out<s32> out, const tma::Path &path, bool case_sensitive),                                                          (out, path, case_sensitive))                                \
    AMS_SF_METHOD_INFO(C, H,  5, Result, OpenDirectory,        (sf::Out<sf::SharedPointer<tma::IDirectoryAccessor>> out, const tma::Path &path, s32 open_mode, bool case_sensitive),    (out, path, open_mode, case_sensitive))                     \
    AMS_SF_METHOD_INFO(C, H,  6, Result, DirectoryExists,      (sf::Out<bool> out, const tma::Path &path, bool case_sensitive),                                                         (out, path, case_sensitive))                                \
    AMS_SF_METHOD_INFO(C, H,  7, Result, CreateDirectory,      (const tma::Path &path, bool case_sensitive),                                                                            (path, case_sensitive))                                     \
    AMS_SF_METHOD_INFO(C, H,  8, Result, DeleteDirectory,      (const tma::Path &path, bool recursively, bool case_sensitive),                                                          (path, recursively, case_sensitive))                        \
    AMS_SF_METHOD_INFO(C, H,  9, Result, RenameDirectory,      (const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive),                                             (old_path, new_path, case_sensitive))                       \
    AMS_SF_METHOD_INFO(C, H, 10, Result, CreateFile,           (const tma::Path &path, s64 size, bool case_sensitive),                                                                  (path, size, case_sensitive))                               \
    AMS_SF_METHOD_INFO(C, H, 11, Result, GetFileTimeStamp,     (sf::Out<u64> out_create, sf::Out<u64> out_access, sf::Out<u64> out_modify, const tma::Path &path, bool case_sensitive), (out_create, out_access, out_modify, path, case_sensitive)) \
    AMS_SF_METHOD_INFO(C, H, 12, Result, GetCaseSensitivePath, (const tma::Path &path, const sf::OutBuffer &out),                                                                       (path, out))                                                \
    AMS_SF_METHOD_INFO(C, H, 13, Result, GetDiskFreeSpaceExW,  (sf::Out<s64> out_free, sf::Out<s64> out_total, sf::Out<s64> out_total_free, const tma::Path &path),                     (out_free, out_total, out_total_free, path))

AMS_SF_DEFINE_INTERFACE(ams::tma, IFileManager, AMS_TMA_I_FILE_MANAGER_INTERFACE_INFO)

/* Prior to system version 6.0.0, case sensitivity was not parameterized. */
#define AMS_TMA_I_DEPRECATED_FILE_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                   \
    AMS_SF_METHOD_INFO(C, H,  0, Result, OpenFile,             (sf::Out<sf::SharedPointer<tma::IFileAccessor>> out, const tma::Path &path, u32 open_mode),          (out, path, open_mode))                      \
    AMS_SF_METHOD_INFO(C, H,  1, Result, FileExists,           (sf::Out<bool> out, const tma::Path &path),                                                          (out, path))                                 \
    AMS_SF_METHOD_INFO(C, H,  2, Result, DeleteFile,           (const tma::Path &path),                                                                             (path))                                      \
    AMS_SF_METHOD_INFO(C, H,  3, Result, RenameFile,           (const tma::Path &old_path, const tma::Path &new_path),                                              (old_path, new_path))                        \
    AMS_SF_METHOD_INFO(C, H,  4, Result, GetIOType,            (sf::Out<s32> out, const tma::Path &path),                                                           (out, path))                                 \
    AMS_SF_METHOD_INFO(C, H,  5, Result, OpenDirectory,        (sf::Out<sf::SharedPointer<tma::IDirectoryAccessor>> out, const tma::Path &path, s32 open_mode),     (out, path, open_mode))                      \
    AMS_SF_METHOD_INFO(C, H,  6, Result, DirectoryExists,      (sf::Out<bool> out, const tma::Path &path),                                                          (out, path))                                 \
    AMS_SF_METHOD_INFO(C, H,  7, Result, CreateDirectory,      (const tma::Path &path),                                                                             (path))                                      \
    AMS_SF_METHOD_INFO(C, H,  8, Result, DeleteDirectory,      (const tma::Path &path, bool recursively),                                                           (path, recursively))                         \
    AMS_SF_METHOD_INFO(C, H,  9, Result, RenameDirectory,      (const tma::Path &old_path, const tma::Path &new_path),                                              (old_path, new_path))                        \
    AMS_SF_METHOD_INFO(C, H, 10, Result, CreateFile,           (const tma::Path &path, s64 size),                                                                   (path, size))                                \
    AMS_SF_METHOD_INFO(C, H, 11, Result, GetFileTimeStamp,     (sf::Out<u64> out_create, sf::Out<u64> out_access, sf::Out<u64> out_modify, const tma::Path &path),  (out_create, out_access, out_modify, path))  \
    AMS_SF_METHOD_INFO(C, H, 12, Result, GetCaseSensitivePath, (const tma::Path &path, const sf::OutBuffer &out),                                                   (path, out))                                 \
    AMS_SF_METHOD_INFO(C, H, 13, Result, GetDiskFreeSpaceExW,  (sf::Out<s64> out_free, sf::Out<s64> out_total, sf::Out<s64> out_total_free, const tma::Path &path), (out_free, out_total, out_total_free, path))

AMS_SF_DEFINE_INTERFACE(ams::tma, IDeprecatedFileManager, AMS_TMA_I_DEPRECATED_FILE_MANAGER_INTERFACE_INFO)
