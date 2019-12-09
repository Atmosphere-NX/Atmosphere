/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

    enum OpenMode {
        OpenMode_Read   = ::FsOpenMode_Read,
        OpenMode_Write  = ::FsOpenMode_Write,
        OpenMode_Append = ::FsOpenMode_Append,

        OpenMode_ReadWrite = (OpenMode_Read | OpenMode_Write),
        OpenMode_All       = (OpenMode_ReadWrite | OpenMode_Append),
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

    using FileTimeStampRaw = ::FsTimeStampRaw;

}
