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
    
    TimeoutHelper auto_reboot_helper(config->fatal_auto_reboot_interval * 1000000);
    
    bool check_vol_up = true, check_vol_down = true;
    GpioPadSession vol_up_btn, vol_down_btn;
    if (R_FAILED(gpioOpenSession(&vol_up_btn, GpioPadName_ButtonVolUp))) {
        check_vol_up = false;
    }
    if (R_FAILED(gpioOpenSession(&vol_down_btn, GpioPadName_ButtonVolDown))) {
        check_vol_down = false;
    }
    
    /* Ensure we close on early return. */
    ON_SCOPE_EXIT { if (check_vol_up) { gpioPadClose(&vol_up_btn); } };
    ON_SCOPE_EXIT { if (check_vol_down) { gpioPadClose(&vol_down_btn); } };
    
    /* Set direction input. */
    if (check_vol_up) {
        gpioPadSetDirection(&vol_up_btn, GpioDirection_Input);
    }
    if (check_vol_down) {
        gpioPadSetDirection(&vol_down_btn, GpioDirection_Input);
    }
    
    BpcSleepButtonState state;
    GpioValue val;
    while (true) {
        Result rc = ResultSuccess;

        if (config->is_auto_reboot_enabled && auto_reboot_helper.TimedOut() ) {
            bpcRebootSystem();
            return;
        }
        
        if (check_vol_up && R_SUCCEEDED((rc = gpioPadGetValue(&vol_up_btn, &val))) && val == GpioValue_Low) {
            bpcRebootSystem();
        }
        
        if (check_vol_down && R_SUCCEEDED((rc = gpioPadGetValue(&vol_down_btn, &val))) && val == GpioValue_Low) {
            bpcRebootSystem();
        }
        
        if ((R_SUCCEEDED(rc = bpcGetSleepButtonState(&state)) && state == BpcSleepButtonState_Held) || (config->quest_flag && reboot_helper.TimedOut())) {
            bpcRebootSystem();
            return;
        }
        
        /* Wait 100 ms between button checks. */
        svcSleepThread(100000000UL);
    }
}

Result PowerControlTask::Run() {
    MonitorBatteryState();
    return ResultSuccess;
}

Result PowerButtonObserveTask::Run() {
    WaitForPowerButton();
    return ResultSuccess;
}

Result StateTransitionStopTask::Run() {
    /* Nintendo ignores the output of this call... */
    spsmPutErrorState();
    return ResultSuccess;
}
