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
#include <stratosphere.hpp>
#include "fatal_types.hpp"
#include "fatal_repair.hpp"
#include "fatal_throw.hpp"

static bool InRepairWithoutVolHeld() {
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_300) {
        return false;
    }
    
    bool in_repair;
    if (R_FAILED(setsysGetFlag(SetSysFlag_InRepairProcessEnable, &in_repair)) || !in_repair) {
        return false;
    }
    
    {
        GpioPadSession vol_btn;
        if (R_FAILED(gpioOpenSession(&vol_btn, GpioPadName_ButtonVolUp))) {
            return true;
        }
        
        /* Ensure we close even on early return. */
        ON_SCOPE_EXIT { gpioPadClose(&vol_btn); };
        
        /* Set direction input. */
        gpioPadSetDirection(&vol_btn, GpioDirection_Input);

        /* Ensure that we're holding the volume button for a full second. */
        TimeoutHelper timeout_helper(1000000000UL);
        while (!timeout_helper.TimedOut()) {
            GpioValue val;
            if (R_FAILED(gpioPadGetValue(&vol_btn, &val)) || val != GpioValue_Low) {
                return true;
            }
            
            /* Sleep for 100 ms. */
            svcSleepThread(100000000UL);
        }
    }
    
    return false;
}

static bool InRepairWithoutTimeReviserCartridge() {
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_500) {
        return false;
    }
    
    bool requires_time_reviser;
    if (R_FAILED(setsysGetFlag(SetSysFlag_RequiresRunRepairTimeReviser, &requires_time_reviser)) || !requires_time_reviser) {
        return false;
    }
    
    FsGameCardHandle gc_hnd;
    u8 gc_attr;
    {
        FsDeviceOperator devop;
        if (R_FAILED(fsOpenDeviceOperator(&devop))) {
            return true;
        }
        
        /* Ensure we close even on early return. */
        ON_SCOPE_EXIT { fsDeviceOperatorClose(&devop); };
        
        /* Check that a gamecard is inserted. */
        bool inserted;
        if (R_FAILED(fsDeviceOperatorIsGameCardInserted(&devop, &inserted)) || !inserted) {
            return true;
        }
        
        /* Check that we can retrieve the gamecard's attributes. */
        if (R_FAILED(fsDeviceOperatorGetGameCardHandle(&devop, &gc_hnd)) || R_FAILED(fsDeviceOperatorGetGameCardAttribute(&devop, &gc_hnd, &gc_attr))) {
            return true;
        }
    }
        
    /* Check that the gamecard is a repair tool. */
    return (gc_attr & FsGameCardAttribute_Repair) == FsGameCardAttribute_Repair;
}

void CheckRepairStatus() {
    if (InRepairWithoutVolHeld()) {
        ThrowFatalForSelf(FatalResult_InRepairWithoutVolHeld);
    }
    
    if (InRepairWithoutTimeReviserCartridge()) {
        ThrowFatalForSelf(FatalResult_InRepairWithoutTimeReviserCartridge);
    }
}
