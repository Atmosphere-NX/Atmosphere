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
#include "fatal_task_power.hpp"

void PowerButtonObserveTask::WaitForPowerButton() {
    /* Wait up to a second for error report generation to finish. */
    eventWait(this->erpt_event, TimeoutHelper::NsToTick(1000000000UL));
    
    /* TODO: Force a reboot after some time if kiosk unit. */
    
    BpcSleepButtonState state;
    while (true) {
        Result rc = bpcGetSleepButtonState(&state);
        if (R_SUCCEEDED(rc) && state == BpcSleepButtonState_Held) {
            bpcRebootSystem();
            return;
        }
        
        /* Wait 100 ms between button checks. */
        svcSleepThread(TimeoutHelper::NsToTick(100000000UL));
    }
}

Result PowerButtonObserveTask::Run() {
    WaitForPowerButton();
    return 0;
}

Result StateTransitionStopTask::Run() {
    /* Nintendo ignores the output of this call... */
    spsmPutErrorState();
    return 0;
}
