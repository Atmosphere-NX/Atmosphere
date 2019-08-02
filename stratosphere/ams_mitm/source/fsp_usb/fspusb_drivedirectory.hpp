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

#include "../fs_mitm/fs_idirectory.hpp"
#include "fspusb_drive.hpp"

class DriveDirectory : public IDirectory {
    private:
        DIR dp;
        s32 usbid;
        
    public:
        DriveDirectory(DIR fatdp, DriveData *drv) : dp(fatdp) {
            usbid = drv->usbif.ID;
        }

        ~DriveDirectory() {
            f_closedir(&dp);
        }

        bool IsOk();
        
    public:
        virtual Result ReadImpl(uint64_t *out_count, FsDirectoryEntry *out_entries, uint64_t max_entries) override;
        virtual Result GetEntryCountImpl(uint64_t *count) override;
};