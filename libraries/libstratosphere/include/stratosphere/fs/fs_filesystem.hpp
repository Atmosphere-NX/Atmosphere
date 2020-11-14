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
#include "fs_common.hpp"

namespace ams::fs {

    namespace fsa {

        class IFile;

    }

    enum OpenMode {
        OpenMode_Read   = (1 << 0),
        OpenMode_Write  = (1 << 1),
        OpenMode_AllowAppend = (1 << 2),

        OpenMode_ReadWrite = (OpenMode_Read | OpenMode_Write),
        OpenMode_All       = (OpenMode_ReadWrite | OpenMode_AllowAppend),
    };

    enum OpenDirectoryMode {
        OpenDirectoryMode_Directory = ::FsDirOpenMode_ReadDirs,
        OpenDirectoryMode_File      = ::FsDirOpenMode_ReadFiles,

        OpenDirectoryMode_All = (OpenDirectoryMode_Directory | OpenDirectoryMode_File),

        /* TODO: Separate enum, like N? */
        OpenDirectoryMode_NotRequireFileSize = ::FsDirOpenMode_NoFileSize,
    };

    enum DirectoryEntryType {
        DirectoryEntryType_Directory = ::FsDirEntryType_Dir,
        DirectoryEntryType_File      = ::FsDirEntryType_File,
    };

    enum CreateOption {
        CreateOption_None    = 0,
        CreateOption_BigFile = ::FsCreateOption_BigFile,
    };

    struct FileHandle;
    struct DirectoryHandle;

    Result CreateFile(const char *path, s64 size);
    Result CreateFile(const char* path, s64 size, int option);
    Result DeleteFile(const char *path);
    Result CreateDirectory(const char *path);
    Result DeleteDirectory(const char *path);
    Result DeleteDirectoryRecursively(const char *path);
    Result RenameFile(const char *old_path, const char *new_path);
    Result RenameDirectory(const char *old_path, const char *new_path);
    Result GetEntryType(DirectoryEntryType *out, const char *path);
    Result OpenFile(FileHandle *out_file, const char *path, int mode);
    Result OpenDirectory(DirectoryHandle *out_dir, const char *path, int mode);
    Result CleanDirectoryRecursively(const char *path);
    Result GetFreeSpaceSize(s64 *out, const char *path);
    Result GetTotalSpaceSize(s64 *out, const char *path);

    Result SetConcatenationFileAttribute(const char *path);

    Result OpenFile(FileHandle *out, std::unique_ptr<fsa::IFile> &&file, int mode);

}
