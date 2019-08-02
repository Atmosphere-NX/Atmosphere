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
#include "fatfs/ff.h"
#include "fspusb_scsi_context.hpp"

static constexpr u32 DriveMax = 10; // FATFS drive limit

#define DRIVES_SCOPE_GUARD std::scoped_lock<HosMutex> lck(g_usbdrive_drives_lock);

class FspUsbResults {
    public:
        static constexpr u32 Module = 476;
        
        static constexpr Result InvalidDriveIndex = MAKERESULT(Module, 1);
        static constexpr Result DriveUnavailable = MAKERESULT(Module, 2);
        static constexpr Result MakeFATFSErrorResult(FRESULT Res) { // Since most operations are FS one with FATFS, had to make this
            if(Res == FR_OK) {
                return 0;
            }
            return MAKERESULT(Module, 100 + (u32)Res);
        }
};

struct DriveData {
    UsbHsClientIfSession usbif;
    UsbHsClientEpSession usbinep;
    UsbHsClientEpSession usboutep;
    std::shared_ptr<SCSIDevice> device;
    std::shared_ptr<SCSIBlock> scsi;
    bool status;
    FATFS fatfs;
    char mountname[0x10];
};

class USBDriveSystem {
    public:
        static Result Initialize();
        static bool IsInitialized();
        static void Update();
        static Result WaitForDrives(s64 timeout = -1);
        static void Finalize();
        static u32 GetDriveCount();
        static u8 GetFSType(u32 index);
};