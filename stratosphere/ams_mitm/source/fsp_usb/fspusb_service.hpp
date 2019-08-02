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

#include "fspusb_drive.hpp"
#include "fspusb_drivefilesystem.hpp"

class FspUsbService final : public IServiceObject {
    protected:
        enum class CommandId {
            UpdateDrives = 0,
            OpenDriveFileSystem = 1,
            GetDriveLabel = 2,
            SetDriveLabel = 3,
            GetDriveFileSystemType = 4,
        };
        
        Result UpdateDrives(Out<u32> count);
        Result OpenDriveFileSystem(u32 index, Out<std::shared_ptr<IFileSystemInterface>> out);
        Result GetDriveLabel(u32 index, OutBuffer<char> out);
        Result SetDriveLabel(u32 index, InBuffer<char> label);
        Result GetDriveFileSystemType(u32 index, Out<u32> out);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(FspUsbService, UpdateDrives),
            MAKE_SERVICE_COMMAND_META(FspUsbService, OpenDriveFileSystem),
            MAKE_SERVICE_COMMAND_META(FspUsbService, GetDriveLabel),
            MAKE_SERVICE_COMMAND_META(FspUsbService, SetDriveLabel),
            MAKE_SERVICE_COMMAND_META(FspUsbService, GetDriveFileSystemType)
        };
};
