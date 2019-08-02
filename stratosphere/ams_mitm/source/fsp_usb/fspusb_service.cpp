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
 
#include <switch.h>
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>

#include "fspusb_service.hpp"

Result FspUsbService::UpdateDrives(Out<u32> count) {
    USBDriveSystem::Update();
    u32 drvcount = USBDriveSystem::GetDriveCount();
    count.SetValue(drvcount);
    return 0;
}

Result FspUsbService::OpenDriveFileSystem(u32 index, Out<std::shared_ptr<IFileSystemInterface>> out) {
    if(index >= USBDriveSystem::GetDriveCount()) {
        return FspUsbResults::InvalidDriveIndex;
    }
    out.SetValue(std::make_shared<IFileSystemInterface>(std::make_shared<DriveFileSystem>(index)));
    return 0;
}

Result FspUsbService::GetDriveLabel(u32 index, OutBuffer<char> out) {
    if(index >= USBDriveSystem::GetDriveCount()) {
        return FspUsbResults::InvalidDriveIndex;
    }
    char drivemount[0x10] = {0};
    sprintf(drivemount, "%d:", index);
    auto res = f_getlabel(drivemount, out.buffer, NULL);
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result FspUsbService::SetDriveLabel(u32 index, InBuffer<char> label) {
    if(index >= USBDriveSystem::GetDriveCount()) {
        return FspUsbResults::InvalidDriveIndex;
    }
    char drivestr[0x100] = {0};
    sprintf(drivestr, "usb-%d:%s", index, label.buffer);
    auto res = f_setlabel(drivestr);
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result FspUsbService::GetDriveFileSystemType(u32 index, Out<u32> out) {
    if(index >= USBDriveSystem::GetDriveCount()) {
        return FspUsbResults::InvalidDriveIndex;
    }
    u8 type = USBDriveSystem::GetFSType(index);
    out.SetValue((u32)type);
    return 0;
}