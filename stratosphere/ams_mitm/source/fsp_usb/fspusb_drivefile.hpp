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

#include "../fs_mitm/fs_ifile.hpp"
#include "fspusb_drive.hpp"

class DriveFile : public IFile {
    private:
        FIL fp;
        s32 usbid;
        
    public:
        DriveFile(FIL fatfp, DriveData *drv) : fp(fatfp) {
            usbid = drv->usbif.ID;
        }

        ~DriveFile() {
            f_close(&fp);
        }

        bool IsOk();
        
    public:
        virtual Result ReadImpl(u64 *out, u64 offset, void *buffer, u64 size) override;
        virtual Result GetSizeImpl(u64 *out) override;
        virtual Result FlushImpl() override;
        virtual Result WriteImpl(u64 offset, void *buffer, u64 size, u32 option) override;
        virtual Result SetSizeImpl(u64 size) override;
        virtual Result OperateRangeImpl(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override;
};