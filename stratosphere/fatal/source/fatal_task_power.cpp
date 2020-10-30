/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
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

        class RebootTimingObserver {
            private:
                os::Tick start_tick;
                TimeSpan interval;
                bool flag;
            public:
                RebootTimingObserver(bool flag, TimeSpan iv) : start_tick(os::GetSystemTick()), interval(iv), flag(flag)  {
                    /* ... */
                }

                bool IsRebootTiming() const {
                    auto current_tick = os::GetSystemTick();
                    return this->flag && (current_tick - this->start_tick).ToTimeSpan() >= this->interval;
                }
        };

        /* Task globals. */
        PowerControlTask g_power_control_task;
        PowerButtonObserveTask g_power_button_observe_task;
        StateTransitionStopTask g_state_transition_stop_task;

        /* Task Implementations. */
        bool PowerControlTask::TryShutdown() {
            /* Set a timeout of 30 seconds. */
            constexpr auto MaxShutdownWaitInterval = TimeSpan::FromSeconds(30);

            auto start_tick = os::GetSystemTick();

            bool perform_shutdown = true;
            PsmBatteryVoltageState bv_state = PsmBatteryVoltageState_Normal;

            while (true) {
                auto cur_tick = os::GetSystemTick();
                if ((cur_tick - start_tick).ToTimeSpan() > MaxShutdownWaitInterval) {
                    break;
                }

                if (R_FAILED(psmGetBatteryVoltageState(&bv_state)) || bv_state == PsmBatteryVoltageState_NeedsShutdown) {
                    break;
                }

                if (bv_state == PsmBatteryVoltageState_Normal) {
                    perform_shutdown = false;
                    break;
                }

                /* Query voltage state every 1 seconds, for 30 seconds. */
                os::SleepThread(TimeSpan::FromSeconds(1));
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
                this->context->erpt_event->TimedWait(TimeSpan::FromSeconds(1));
                this->TryShutdown();
                return;
            }

            /* Signal we've checked the battery at least once. */
            this->context->battery_event->Signal();

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

                os::SleepThread(TimeSpan::FromSeconds(5));
            }
        }

        void PowerButtonObserveTask::WaitForPowerButton() {
            /* Wait up to a second for error report generation to finish. */
            this->context->erpt_event->TimedWait(TimeSpan::FromSeconds(1));

            /* Force a reboot after some time if kiosk unit. */
            const auto &config = GetFatalConfig();
            RebootTimingObserver quest_reboot_helper(config.IsQuest(), config.GetQuestRebootTimeoutInterval());
            RebootTimingObserver fatal_reboot_helper(config.IsFatalRebootEnabled(), config.GetFatalRebootTimeoutInterval());

            bool check_vol_up = true, check_vol_down = true;
            gpio::GpioPadSession vol_up_btn, vol_down_btn;
            if (R_FAILED(gpio::OpenSession(std::addressof(vol_up_btn), gpio::DeviceCode_ButtonVolUp))) {
                check_vol_up = false;
            }
            if (R_FAILED(gpio::OpenSession(std::addressof(vol_down_btn), gpio::DeviceCode_ButtonVolDn))) {
                check_vol_down = false;
            }

            /* Ensure we close on early return. */
            ON_SCOPE_EXIT { if (check_vol_up)   { gpio::CloseSession(std::addressof(vol_up_btn)); } };
            ON_SCOPE_EXIT { if (check_vol_down) { gpio::CloseSession(std::addressof(vol_down_btn)); } };

            /* Set direction input. */
            if (check_vol_up) {
                gpio::SetDirection(std::addressof(vol_up_btn), gpio::Direction_Input);
            }
            if (check_vol_down) {
                gpio::SetDirection(std::addressof(vol_down_btn), gpio::Direction_Input);
            }

            BpcSleepButtonState state;
            while (true) {
                if (fatal_reboot_helper.IsRebootTiming() || (quest_reboot_helper.IsRebootTiming()) ||
                    (check_vol_up && gpio::GetValue(std::addressof(vol_up_btn)) == gpio::GpioValue_Low) ||
                    (check_vol_down && gpio::GetValue(std::addressof(vol_down_btn)) == gpio::GpioValue_Low) ||
                    (R_SUCCEEDED(bpcGetSleepButtonState(&state)) && state == BpcSleepButtonState_Held))
                {
                    /* If any of the above conditions succeeded, we should reboot. */
                    bpcRebootSystem();
                    return;
                }


                /* Wait 100 ms between button checks. */
                os::SleepThread(TimeSpan::FromMilliSeconds(100));
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
