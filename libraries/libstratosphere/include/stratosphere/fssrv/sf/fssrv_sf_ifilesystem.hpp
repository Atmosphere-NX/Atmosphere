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
#pragma once
#include <vapours.hpp>
#include <stratosphere/sf.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_filesystem_for_debug.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_path.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_ifile.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_idirectory.hpp>

namespace ams::fssrv::sf {

    #define AMS_FSSRV_I_FILESYSTEM_INTERFACE_INFO(C, H)                                                                                                                                                            \
        AMS_SF_METHOD_INFO(C, H,  0, Result, CreateFile,                 (const ams::fssrv::sf::Path &path, s64 size, s32 option))                                                                                 \
        AMS_SF_METHOD_INFO(C, H,  1, Result, DeleteFile,                 (const ams::fssrv::sf::Path &path))                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  2, Result, CreateDirectory,            (const ams::fssrv::sf::Path &path))                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  3, Result, DeleteDirectory,            (const ams::fssrv::sf::Path &path))                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  4, Result, DeleteDirectoryRecursively, (const ams::fssrv::sf::Path &path))                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  5, Result, RenameFile,                 (const ams::fssrv::sf::Path &old_path, const ams::fssrv::sf::Path &new_path))                                                             \
        AMS_SF_METHOD_INFO(C, H,  6, Result, RenameDirectory,            (const ams::fssrv::sf::Path &old_path, const ams::fssrv::sf::Path &new_path))                                                             \
        AMS_SF_METHOD_INFO(C, H,  7, Result, GetEntryType,               (ams::sf::Out<u32> out, const ams::fssrv::sf::Path &path))                                                                                \
        AMS_SF_METHOD_INFO(C, H,  8, Result, OpenFile,                   (ams::sf::Out<std::shared_ptr<ams::fssrv::sf::IFile>> out, const ams::fssrv::sf::Path &path, u32 mode))                                   \
        AMS_SF_METHOD_INFO(C, H,  9, Result, OpenDirectory,              (ams::sf::Out<std::shared_ptr<ams::fssrv::sf::IDirectory>> out, const ams::fssrv::sf::Path &path, u32 mode))                              \
        AMS_SF_METHOD_INFO(C, H, 10, Result, Commit,                     ())                                                                                                                                       \
        AMS_SF_METHOD_INFO(C, H, 11, Result, GetFreeSpaceSize,           (ams::sf::Out<s64> out, const ams::fssrv::sf::Path &path))                                                                                \
        AMS_SF_METHOD_INFO(C, H, 12, Result, GetTotalSpaceSize,          (ams::sf::Out<s64> out, const ams::fssrv::sf::Path &path))                                                                                \
        AMS_SF_METHOD_INFO(C, H, 13, Result, CleanDirectoryRecursively,  (const ams::fssrv::sf::Path &path),                                                                                   hos::Version_3_0_0) \
        AMS_SF_METHOD_INFO(C, H, 14, Result, GetFileTimeStampRaw,        (ams::sf::Out<ams::fs::FileTimeStampRaw> out, const ams::fssrv::sf::Path &path),                                      hos::Version_3_0_0) \
        AMS_SF_METHOD_INFO(C, H, 15, Result, QueryEntry,                 (const ams::sf::OutBuffer &out_buf, const ams::sf::InBuffer &in_buf, s32 query_id, const ams::fssrv::sf::Path &path), hos::Version_4_0_0)

    AMS_SF_DEFINE_INTERFACE(IFileSystem, AMS_FSSRV_I_FILESYSTEM_INTERFACE_INFO)

}
