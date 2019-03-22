/*
 * Copyright (c) 2018 Atmosphère-NX
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
    
enum OpenMode {
    OpenMode_Read = (1 << 0),
    OpenMode_Write = (1 << 1),
    OpenMode_Append = (1 << 2),
    
    OpenMode_ReadWrite = OpenMode_Read | OpenMode_Write,
    OpenMode_All = OpenMode_ReadWrite | OpenMode_Append,
};

enum DirectoryOpenMode {
    DirectoryOpenMode_Directories = (1 << 0),
    DirectoryOpenMode_Files = (1 << 1),
    
    DirectoryOpenMode_All = (DirectoryOpenMode_Directories | DirectoryOpenMode_Files),
};

enum DirectoryEntryType {
    DirectoryEntryType_Directory,
    DirectoryEntryType_File,
};
