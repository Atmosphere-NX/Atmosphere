/*
 * Copyright (c) Atmosphère-NX
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

    class BatteryDevice : public powctl::impl::IDevice {
        NON_COPYABLE(BatteryDevice);
        NON_MOVEABLE(BatteryDevice);
        AMS_DDSF_CASTABLE_TRAITS(ams::powctl::impl::board::nintendo::nx::BatteryDevice, ::ams::powctl::impl::IDevice);
        private:
            bool m_use_event_handler;
            util::optional<BatteryInterruptEventHandler> m_event_handler;
            os::SystemEventType m_system_event;
        public:
            BatteryDevice(bool ev);

            os::SystemEventType *GetSystemEvent() { return std::addressof(m_system_event); }

            void SetInterruptEnabled(bool en) {
                if (m_use_event_handler) {
                    m_event_handler->SetInterruptEnabled(en);
                }
            }
    };

    class BatteryDriver : public IPowerControlDriver {
        NON_COPYABLE(BatteryDriver);
        NON_MOVEABLE(BatteryDriver);
        AMS_DDSF_CASTABLE_TRAITS(ams::powctl::impl::board::nintendo::nx::BatteryDriver, ::ams::powctl::impl::IPowerControlDriver);
        public:
            BatteryDriver(bool ev) : IPowerControlDriver(ev) { /* ... */ }

            /* Generic API. */
            virtual void InitializeDriver() override;
            virtual void FinalizeDriver() override;

            virtual Result GetDeviceSystemEvent(os::SystemEventType **out, IDevice *device) override;
            virtual Result SetDeviceInterruptEnabled(IDevice *device, bool enable) override;

            virtual Result GetDeviceErrorStatus(u32 *out, IDevice *device) override;
            virtual Result SetDeviceErrorStatus(IDevice *device, u32 status) override;

            /* Battery API. */
            virtual Result GetBatteryChargePercentage(float *out_percent, IDevice *device) override;
            virtual Result GetBatteryVoltageFuelGaugePercentage(float *out_percent, IDevice *device) override;

            virtual Result GetBatteryFullCapacity(int *out_mah, IDevice *device) override;
            virtual Result GetBatteryRemainingCapacity(int *out_mah, IDevice *device) override;

            virtual Result SetBatteryChargePercentageMinimumAlertThreshold(IDevice *device, float percentage) override;
            virtual Result SetBatteryChargePercentageMaximumAlertThreshold(IDevice *device, float percentage) override;

            virtual Result SetBatteryVoltageFuelGaugePercentageMinimumAlertThreshold(IDevice *device, float percentage) override;
            virtual Result SetBatteryVoltageFuelGaugePercentageMaximumAlertThreshold(IDevice *device, float percentage) override;

            virtual Result SetBatteryFullChargeThreshold(IDevice *device, float percentage) override;

            virtual Result GetBatteryAverageCurrent(int *out_ma, IDevice *device) override;
            virtual Result GetBatteryCurrent(int *out_ma, IDevice *device) override;

            virtual Result GetBatteryInternalState(void *dst, size_t *out_size, IDevice *device, size_t dst_size) override;
            virtual Result SetBatteryInternalState(IDevice *device, const void *src, size_t src_size) override;

            virtual Result GetBatteryNeedToRestoreParameters(bool *out, IDevice *device) override;
            virtual Result SetBatteryNeedToRestoreParameters(IDevice *device, bool en) override;

            virtual Result IsBatteryI2cShutdownEnabled(bool *out, IDevice *device) override;
            virtual Result SetBatteryI2cShutdownEnabled(IDevice *device, bool en) override;

            virtual Result IsBatteryPresent(bool *out, IDevice *device) override;

            virtual Result GetBatteryCycles(int *out, IDevice *device) override;
            virtual Result SetBatteryCycles(IDevice *device, int cycles) override;

            virtual Result GetBatteryAge(float *out_percent, IDevice *device) override;

            virtual Result GetBatteryTemperature(float *out_c, IDevice *device) override;
            virtual Result GetBatteryMaximumTemperature(float *out_c, IDevice *device) override;

            virtual Result SetBatteryTemperatureMinimumAlertThreshold(IDevice *device, float c) override;
            virtual Result SetBatteryTemperatureMaximumAlertThreshold(IDevice *device, float c) override;

            virtual Result GetBatteryVCell(int *out_mv, IDevice *device) override;
            virtual Result GetBatteryAverageVCell(int *out_mv, IDevice *device) override;

            virtual Result GetBatteryAverageVCellTime(TimeSpan *out, IDevice *device) override;

            virtual Result SetBatteryVoltageMinimumAlertThreshold(IDevice *device, int mv) override;

            virtual Result GetBatteryOpenCircuitVoltage(int *out_mv, IDevice *device) override;

            virtual Result SetBatteryVoltageMaximumAlertThreshold(IDevice *device, int mv) override;

            /* Unsupported Charger API. */
            virtual Result GetChargerChargeCurrentState(ChargeCurrentState *out, IDevice *device) override { AMS_UNUSED(out, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerChargeCurrentState(IDevice *device, ChargeCurrentState state) override { AMS_UNUSED(device, state); return powctl::ResultNotSupported(); }

            virtual Result GetChargerFastChargeCurrentLimit(int *out_ma, IDevice *device) override { AMS_UNUSED(out_ma, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerFastChargeCurrentLimit(IDevice *device, int ma) override { AMS_UNUSED(device, ma); return powctl::ResultNotSupported(); }

            virtual Result GetChargerChargeVoltageLimit(int *out_mv, IDevice *device) override { AMS_UNUSED(out_mv, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerChargeVoltageLimit(IDevice *device, int mv) override { AMS_UNUSED(device, mv); return powctl::ResultNotSupported(); }

            virtual Result SetChargerChargerConfiguration(IDevice *device, ChargerConfiguration cfg) override { AMS_UNUSED(device, cfg); return powctl::ResultNotSupported(); }

            virtual Result IsChargerHiZEnabled(bool *out, IDevice *device) override { AMS_UNUSED(out, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerHiZEnabled(IDevice *device, bool en) override { AMS_UNUSED(device, en); return powctl::ResultNotSupported(); }

            virtual Result GetChargerInputCurrentLimit(int *out_ma, IDevice *device) override { AMS_UNUSED(out_ma, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerInputCurrentLimit(IDevice *device, int ma) override { AMS_UNUSED(device, ma); return powctl::ResultNotSupported(); }

            virtual Result SetChargerInputVoltageLimit(IDevice *device, int mv) override { AMS_UNUSED(device, mv); return powctl::ResultNotSupported(); }

            virtual Result SetChargerBoostModeCurrentLimit(IDevice *device, int ma) override { AMS_UNUSED(device, ma); return powctl::ResultNotSupported(); }

            virtual Result GetChargerChargerStatus(ChargerStatus *out, IDevice *device) override { AMS_UNUSED(out, device); return powctl::ResultNotSupported(); }

            virtual Result IsChargerWatchdogTimerEnabled(bool *out, IDevice *device) override { AMS_UNUSED(out, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerWatchdogTimerEnabled(IDevice *device, bool en) override { AMS_UNUSED(device, en); return powctl::ResultNotSupported(); }

            virtual Result SetChargerWatchdogTimerTimeout(IDevice *device, TimeSpan timeout) override { AMS_UNUSED(device, timeout); return powctl::ResultNotSupported(); }
            virtual Result ResetChargerWatchdogTimer(IDevice *device) override { AMS_UNUSED(device); return powctl::ResultNotSupported(); }

            virtual Result GetChargerBatteryCompensation(int *out_mo, IDevice *device) override { AMS_UNUSED(out_mo, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerBatteryCompensation(IDevice *device, int mo) override { AMS_UNUSED(device, mo); return powctl::ResultNotSupported(); }

            virtual Result GetChargerVoltageClamp(int *out_mv, IDevice *device) override { AMS_UNUSED(out_mv, device); return powctl::ResultNotSupported(); }
            virtual Result SetChargerVoltageClamp(IDevice *device, int mv) override { AMS_UNUSED(device, mv); return powctl::ResultNotSupported(); }
    };

}
