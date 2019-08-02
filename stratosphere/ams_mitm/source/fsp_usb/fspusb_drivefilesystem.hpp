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
#include <switch.h>
#include <stratosphere.hpp>

#include "../fs_mitm/fs_ifilesystem.hpp"
#include "fspusb_drivefile.hpp"
#include "fspusb_drivedirectory.hpp"

class DriveFileSystem : public IFileSystem {
    private:
        u32 drvidx;
        s32 usbid;
        
    public:
        DriveFileSystem(u32 idx) : drvidx(idx) {
            usbid = GetDriveAccess()->usbif.ID;
        }

        DriveData *GetDriveAccess();
        bool IsOk();

        std::string GetFullPath(const FsPath &path) {
            // Convert fsp path to FATFS path, acording to the drive's mount name.
            DriveData *data = GetDriveAccess();
            if(data == NULL) {
                return "";
            }
            std::string str = std::string(data->mountname) + std::string(path.str);
            // Debug
            printf("GetFullPath (path = \"%s\")\n", str.c_str());
            return str;
        }
        
    public:
        virtual Result CreateFileImpl(const FsPath &path, uint64_t size, int flags) override;
        virtual Result DeleteFileImpl(const FsPath &path) override;
        virtual Result CreateDirectoryImpl(const FsPath &path) override;
        virtual Result DeleteDirectoryImpl(const FsPath &path) override;
        virtual Result DeleteDirectoryRecursivelyImpl(const FsPath &path) override;
        virtual Result RenameFileImpl(const FsPath &old_path, const FsPath &new_path) override;
        virtual Result RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) override;
        virtual Result GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) override;
        virtual Result OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) override;
        virtual Result OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) override;
        virtual Result CommitImpl() override;
        virtual Result GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) override;
        virtual Result GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) override;
        virtual Result CleanDirectoryRecursivelyImpl(const FsPath &path) override;
        virtual Result GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) override;
        virtual Result QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) override;
};