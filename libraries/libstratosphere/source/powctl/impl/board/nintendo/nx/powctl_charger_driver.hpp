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
#pragma once
#include <stratosphere.hpp>
#include "../../../powctl_i_power_control_driver.hpp"
#include "powctl_interrupt_event_handler.hpp"

namespace ams::powctl::impl::board::nintendo::nx {

    class ChargerDevice : public powctl::impl::IDevice {
        NON_COPYABLE(ChargerDevice);
        NON_MOVEABLE(ChargerDevice);
        AMS_DDSF_CASTABLE_TRAITS(ams::powctl::impl::board::nintendo::nx::ChargerDevice, ::ams::powctl::impl::IDevice);
        private:
            gpio::GpioPadSession gpio_pad_session;
            bool watchdog_timer_enabled;
            TimeSpan watchdog_timer_timeout;
            bool use_event_handler;
            std::optional<ChargerInterruptEventHandler> event_handler;
            os::SystemEventType system_event;
        public:
            ChargerDevice(bool ev);

            bool IsWatchdogTimerEnabled() const { return this->watchdog_timer_enabled; }
            void SetWatchdogTimerEnabled(bool en) { this->watchdog_timer_enabled = en; }

            TimeSpan GetWatchdogTimerTimeout() const { return this->watchdog_timer_timeout; }
            void SetWatchdogTimerTimeout(TimeSpan ts) { this->watchdog_timer_timeout = ts; }

            gpio::GpioPadSession *GetPadSession() { return std::addressof(this->gpio_pad_session); }

            os::SystemEventType *GetSystemEvent() { return std::addressof(this->system_event); }

            void SetInterruptEnabled(bool en) {
                if (this->use_event_handler) {
                    this->event_handler->SetInterruptEnabled(en);
                }
            }
    };

    class ChargerDriver : public IPowerControlDriver {
        NON_COPYABLE(ChargerDriver);
        NON_MOVEABLE(ChargerDriver);
        AMS_DDSF_CASTABLE_TRAITS(ams::powctl::impl::board::nintendo::nx::ChargerDriver, ::ams::powctl::impl::IPowerControlDriver);
        public:
            ChargerDriver(bool ev) : IPowerControlDriver(ev) { /* ... */ }

            /* Generic API. */
            virtual void InitializeDriver() override;
            virtual void FinalizeDriver() override;

            virtual Result GetDeviceSystemEvent(os::SystemEventType **out, IDevice *device) override;
            virtual Result SetDeviceInterruptEnabled(IDevice *device, bool enable) override;

            virtual Result GetDeviceErrorStatus(u32 *out, IDevice *device) override;
            virtual Result SetDeviceErrorStatus(IDevice *device, u32 status) override;

            /* Charger API. */
            virtual Result GetChargerChargeCurrentState(ChargeCurrentState *out, IDevice *device) override;
            virtual Result SetChargerChargeCurrentState(IDevice *device, ChargeCurrentState state) override;

            virtual Result GetChargerFastChargeCurrentLimit(int *out_ma, IDevice *device) override;
            virtual Result SetChargerFastChargeCurrentLimit(IDevice *device, int ma) override;

            virtual Result GetChargerChargeVoltageLimit(int *out_mv, IDevice *device) override;
            virtual Result SetChargerChargeVoltageLimit(IDevice *device, int mv) override;

            virtual Result SetChargerChargerConfiguration(IDevice *device, ChargerConfiguration cfg) override;

            virtual Result IsChargerHiZEnabled(bool *out, IDevice *device) override;
            virtual Result SetChargerHiZEnabled(IDevice *device, bool en) override;

            virtual Result GetChargerInputCurrentLimit(int *out_ma, IDevice *device) override;
            virtual Result SetChargerInputCurrentLimit(IDevice *device, int ma) override;

            virtual Result SetChargerInputVoltageLimit(IDevice *device, int mv) override;

            virtual Result SetChargerBoostModeCurrentLimit(IDevice *device, int ma) override;

            virtual Result GetChargerChargerStatus(ChargerStatus *out, IDevice *device) override;

            virtual Result IsChargerWatchdogTimerEnabled(bool *out, IDevice *device) override;
            virtual Result SetChargerWatchdogTimerEnabled(IDevice *device, bool en) override;

            virtual Result SetChargerWatchdogTimerTimeout(IDevice *device, TimeSpan timeout) override;
            virtual Result ResetChargerWatchdogTimer(IDevice *device) override;

            virtual Result GetChargerBatteryCompensation(int *out_mo, IDevice *device) override;
            virtual Result SetChargerBatteryCompensation(IDevice *device, int mo) override;

            virtual Result GetChargerVoltageClamp(int *out_mv, IDevice *device) override;
            virtual Result SetChargerVoltageClamp(IDevice *device, int mv) override;

            /* Unsupported Battery API. */
            virtual Result GetBatterySocRep(float *out_percent, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatterySocVf(float *out_percent, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryFullCapacity(int *out_mah, IDevice *device) override { return powctl::ResultNotSupported(); }
            virtual Result GetBatteryRemainingCapacity(int *out_mah, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result SetBatteryPercentageMinimumAlertThreshold(IDevice *device, float percentage) override { return powctl::ResultNotSupported(); }
            virtual Result SetBatteryPercentageMaximumAlertThreshold(IDevice *device, float percentage) override { return powctl::ResultNotSupported(); }
            virtual Result SetBatteryPercentageFullThreshold(IDevice *device, float percentage) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryAverageCurrent(int *out_ma, IDevice *device) override { return powctl::ResultNotSupported(); }
            virtual Result GetBatteryCurrent(int *out_ma, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryInternalState(void *dst, size_t *out_size, IDevice *device, size_t dst_size) override { return powctl::ResultNotSupported(); }
            virtual Result SetBatteryInternalState(IDevice *device, const void *src, size_t src_size) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryNeedToRestoreParameters(bool *out, IDevice *device) override { return powctl::ResultNotSupported(); }
            virtual Result SetBatteryNeedToRestoreParameters(IDevice *device, bool en) override { return powctl::ResultNotSupported(); }

            virtual Result IsBatteryI2cShutdownEnabled(bool *out, IDevice *device) override { return powctl::ResultNotSupported(); }
            virtual Result SetBatteryI2cShutdownEnabled(IDevice *device, bool en) override { return powctl::ResultNotSupported(); }

            virtual Result IsBatteryPresent(bool *out, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryCycles(int *out, IDevice *device) override { return powctl::ResultNotSupported(); }
            virtual Result SetBatteryCycles(IDevice *device, int cycles) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryAge(float *out_percent, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryTemperature(float *out_c, IDevice *device) override { return powctl::ResultNotSupported(); }
            virtual Result GetBatteryMaximumTemperature(float *out_c, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result SetBatteryTemperatureMinimumAlertThreshold(IDevice *device, float c) override { return powctl::ResultNotSupported(); }
            virtual Result SetBatteryTemperatureMaximumAlertThreshold(IDevice *device, float c) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryVCell(int *out_mv, IDevice *device) override { return powctl::ResultNotSupported(); }
            virtual Result GetBatteryAverageVCell(int *out_mv, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryAverageVCellTime(TimeSpan *out, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result SetBatteryVoltageMinimumAlertThreshold(IDevice *device, int mv) override { return powctl::ResultNotSupported(); }

            virtual Result GetBatteryOpenCircuitVoltage(int *out_mv, IDevice *device) override { return powctl::ResultNotSupported(); }

            virtual Result SetBatteryVoltageMaximumAlertThreshold(IDevice *device, int mv) override { return powctl::ResultNotSupported(); }
    };

}
