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
#include "fatal_config.hpp"

bool PowerControlTask::TryShutdown() {
    /* Set a timeout of 30 seconds. */
    TimeoutHelper timeout_helper(30000000000UL); 
    bool cancel_shutdown = false;
    PsmBatteryVoltageState bv_state = PsmBatteryVoltageState_Normal;
    
    while (true) {
        if (timeout_helper.TimedOut()) {
            break;
        }
        
        if (R_FAILED(psmGetBatteryVoltageState(&bv_state)) || bv_state == PsmBatteryVoltageState_NeedsShutdown) {
            break;
        }
        
        if (bv_state == PsmBatteryVoltageState_Normal) {
            cancel_shutdown = true;
            break;
        }
        
        /* Query voltage state every 5 seconds, for 30 seconds. */
        svcSleepThread(5000000000UL);
    }
    
    if (!cancel_shutdown) {
        bpcShutdownSystem();
        return true;
    } else {
        return false;
    }
}

void PowerControlTask::MonitorBatteryState() {
    PsmBatteryVoltageState bv_state = PsmBatteryVoltageState_Normal;
    
    /* Check the battery state, and shutdown on low voltage. */
    if (R_FAILED(psmGetBatteryVoltageState(&bv_state)) || bv_state == PsmBatteryVoltageState_NeedsShutdown) {
        /* Wait a second for the error report task to finish. */
        eventWait(this->erpt_event, TimeoutHelper::NsToTick(1000000000UL));
        this->TryShutdown();
        return;
    }
    
    /* Signal we've checked the battery at least once. */
    eventFire(this->battery_event);
    
    while (true) {
        if (R_FAILED(psmGetBatteryVoltageState(&bv_state))) {
            bv_state = PsmBatteryVoltageState_NeedsShutdown;
        }
        
        switch (bv_state) {
            case PsmBatteryVoltageState_NeedsShutdown:
            case PsmBatteryVoltageState_NeedsSleep:
                {
                    bool shutdown = this->TryShutdown();
                    if (shutdown) {
                        return;
                    }
                }
                break;
            default:
                break;
        }
        
        /* Query voltage state every 5 seconds. */
        svcSleepThread(5000000000UL);
    }
}

void PowerButtonObserveTask::WaitForPowerButton() {
    /* Wait up to a second for error report generation to finish. */
    eventWait(this->erpt_event, TimeoutHelper::NsToTick(1000000000UL));
    
    /* Force a reboot after some time if kiosk unit. */
    const FatalConfig *config = GetFatalConfig();
    TimeoutHelper reboot_helper(config->quest_reboot_interval_second * 1000000000UL); 
    
    BpcSleepButtonState state;
    while (true) {
        
        
        Result rc = bpcGetSleepButtonState(&state);
        if ((R_SUCCEEDED(rc) && state == BpcSleepButtonState_Held) || (config->quest_flag && reboot_helper.TimedOut())) {
            bpcRebootSystem();
            return;
        }
        
        /* Wait 100 ms between button checks. */
        svcSleepThread(100000000UL);
    }
}

Result PowerControlTask::Run() {
    MonitorBatteryState();
    return 0;
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
