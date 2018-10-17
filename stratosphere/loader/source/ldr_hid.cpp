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
 
#include <switch.h>
#include <string.h>

#include "ldr_content_management.hpp"
#include "ldr_hid.hpp"

Result HidManagement::GetKeysDown(u64 *keys) {
    if (!ContentManagement::HasCreatedTitle(0x0100000000000013) || R_FAILED(hidInitialize())) {
        return MAKERESULT(Module_Libnx, LibnxError_InitFail_HID);
    }
    
    hidScanInput();
    *keys = hidKeysDown(CONTROLLER_P1_AUTO);
    
    hidExit();
    return 0x0;
}