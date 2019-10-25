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
#include "fatal_config.hpp"
#include "fatal_task_power.hpp"

namespace ams::fatal::srv {

    namespace {

        /* Task types. */
        class PowerControlTask : public ITaskWithDefaultStack {
            private:
                bool TryShutdown();
                void MonitorBatteryState();
            public:
                virtual Result Run() override;
                virtual const char *GetName() const override {
                    return "PowerControlTask";
                }
        };

        class PowerButtonObserveTask : public ITaskWithDefaultStack {
            private:
                void WaitForPowerButton();
            public:
                virtual Result Run() override;
                virtual const char *GetName() const override {
                    return "PowerButtonObserveTask";
                }
        };

        class StateTransitionStopTask : public ITaskWithDefaultStack {
            public:
                virtual Result Run() override;
                virtual const char *GetName() const override {
                    return "StateTransitionStopTask";
                }
        };

        /* Task globals. */
        PowerControlTask g_power_control_task;
        PowerButtonObserveTask g_power_button_observe_task;
        StateTransitionStopTask g_state_transition_stop_task;

        /* Task Implementations. */
        bool PowerControlTask::TryShutdown() {
            /* Set a timeout of 30 seconds. */
            os::TimeoutHelper timeout_helper(30'000'000'000ul);

            bool perform_shutdown = true;
            PsmBatteryVoltageState bv_state = PsmBatteryVoltageState_Normal;

            while (true) {
                if (timeout_helper.TimedOut()) {
                    break;
                }

                if (R_FAILED(psmGetBatteryVoltageState(&bv_state)) || bv_state == PsmBatteryVoltageState_NeedsShutdown) {
                    break;
                }

                if (bv_state == PsmBatteryVoltageState_Normal) {
                    perform_shutdown = false;
                    break;
                }

                /* Query voltage state every 5 seconds, for 30 seconds. */
                svcSleepThread(5'000'000'000ul);
            }

            if (perform_shutdown) {
                bpcShutdownSystem();
            }

            return perform_shutdown;
        }

        void PowerControlTask::MonitorBatteryState() {
            PsmBatteryVoltageState bv_state = PsmBatteryVoltageState_Normal;

            /* Check the battery state, and shutdown on low voltage. */
            if (R_FAILED(psmGetBatteryVoltageState(&bv_state)) || bv_state == PsmBatteryVoltageState_NeedsShutdown) {
                /* Wait a second for the error report task to finish. */
                eventWait(const_cast<Event *>(&this->context->erpt_event), os::TimeoutHelper::NsToTick(1'000'000'000ul));
                this->TryShutdown();
                return;
            }

            /* Signal we've checked the battery at least once. */
            eventFire(const_cast<Event *>(&this->context->battery_event));

            /* Loop querying voltage state every 5 seconds. */
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

                svcSleepThread(5'000'000'000ul);
            }
        }

        void PowerButtonObserveTask::WaitForPowerButton() {
            /* Wait up to a second for error report generation to finish. */
            eventWait(const_cast<Event *>(&this->context->erpt_event), os::TimeoutHelper::NsToTick(1'000'000'000ul));

            /* Force a reboot after some time if kiosk unit. */
            const auto &config = GetFatalConfig();
            os::TimeoutHelper quest_reboot_helper(config.GetQuestRebootTimeoutInterval());
            os::TimeoutHelper fatal_reboot_helper(config.GetFatalRebootTimeoutInterval());

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
                if ((config.IsFatalRebootEnabled() && fatal_reboot_helper.TimedOut()) ||
                    (check_vol_up && R_SUCCEEDED(gpioPadGetValue(&vol_up_btn, &val)) && val == GpioValue_Low) ||
                    (check_vol_down && R_SUCCEEDED(gpioPadGetValue(&vol_down_btn, &val)) && val == GpioValue_Low) ||
                    (R_SUCCEEDED(bpcGetSleepButtonState(&state)) && state == BpcSleepButtonState_Held) ||
                    (config.IsQuest() && quest_reboot_helper.TimedOut())) {
                    /* If any of the above conditions succeeded, we should reboot. */
                    bpcRebootSystem();
                    return;
                }


                /* Wait 100 ms between button checks. */
                svcSleepThread(100'000'000ul);
            }
        }

        Result PowerControlTask::Run() {
            this->MonitorBatteryState();
            return ResultSuccess();
        }

        Result PowerButtonObserveTask::Run() {
            this->WaitForPowerButton();
            return ResultSuccess();
        }

        Result StateTransitionStopTask::Run() {
            /* Nintendo ignores the output of this call... */
            spsmPutErrorState();
            return ResultSuccess();
        }

    }

    ITask *GetPowerControlTask(const ThrowContext *ctx) {
        g_power_control_task.Initialize(ctx);
        return &g_power_control_task;
    }

    ITask *GetPowerButtonObserveTask(const ThrowContext *ctx) {
        g_power_button_observe_task.Initialize(ctx);
        return &g_power_button_observe_task;
    }

    ITask *GetStateTransitionStopTask(const ThrowContext *ctx) {
        g_state_transition_stop_task.Initialize(ctx);
        return &g_state_transition_stop_task;
    }

}
