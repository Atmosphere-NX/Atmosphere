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

namespace ams::powctl::impl {

    class IDevice : public ::ams::ddsf::IDevice {
        NON_COPYABLE(IDevice);
        NON_MOVEABLE(IDevice);
        AMS_DDSF_CASTABLE_TRAITS(ams::powctl::impl::IDevice, ::ams::ddsf::IDevice);
        public:
            IDevice() : ddsf::IDevice(false) { /* ... */ }
            virtual ~IDevice() { /* ... */ }
    };

    class IPowerControlDriver : public ::ams::ddsf::IDriver {
        NON_COPYABLE(IPowerControlDriver);
        NON_MOVEABLE(IPowerControlDriver);
        AMS_DDSF_CASTABLE_TRAITS(ams::powctl::impl::IPowerControlDriver, ::ams::ddsf::IDriver);
        private:
            bool event_handler_enabled;
        protected:
            constexpr bool IsEventHandlerEnabled() const {
                return this->event_handler_enabled;
            }
        public:
            IPowerControlDriver(bool ev) : IDriver(), event_handler_enabled(ev) { /* ... */ }
            virtual ~IPowerControlDriver() { /* ... */ }

            virtual void InitializeDriver() = 0;
            virtual void FinalizeDriver() = 0;

            virtual Result GetDeviceSystemEvent(os::SystemEventType **out, IDevice *device) = 0;
            virtual Result SetDeviceInterruptEnabled(IDevice *device, bool enable) = 0;

            /* TODO: Eventually implement proper error status enum? */
            virtual Result GetDeviceErrorStatus(u32 *out, IDevice *device) = 0;
            virtual Result SetDeviceErrorStatus(IDevice *device, u32 status) = 0;

            virtual Result GetBatterySocRep(float *out_percent, IDevice *device) = 0;

            virtual Result GetBatterySocVf(float *out_percent, IDevice *device) = 0;

            virtual Result GetBatteryFullCapacity(int *out_mah, IDevice *device) = 0;
            virtual Result GetBatteryRemainingCapacity(int *out_mah, IDevice *device) = 0;

            virtual Result SetBatteryPercentageMinimumAlertThreshold(IDevice *device, float percentage) = 0;
            virtual Result SetBatteryPercentageMaximumAlertThreshold(IDevice *device, float percentage) = 0;
            virtual Result SetBatteryPercentageFullThreshold(IDevice *device, float percentage) = 0;

            virtual Result GetChargerChargeCurrentState(ChargeCurrentState *out, IDevice *device) = 0;
            virtual Result SetChargerChargeCurrentState(IDevice *device, ChargeCurrentState state) = 0;

            virtual Result GetChargerFastChargeCurrentLimit(int *out_ma, IDevice *device) = 0;
            virtual Result SetChargerFastChargeCurrentLimit(IDevice *device, int ma) = 0;

            virtual Result GetChargerChargeVoltageLimit(int *out_mv, IDevice *device) = 0;
            virtual Result SetChargerChargeVoltageLimit(IDevice *device, int mv) = 0;

            virtual Result SetChargerChargerConfiguration(IDevice *device, ChargerConfiguration cfg) = 0;

            virtual Result IsChargerHiZEnabled(bool *out, IDevice *device) = 0;
            virtual Result SetChargerHiZEnabled(IDevice *device, bool en) = 0;

            virtual Result GetBatteryAverageCurrent(int *out_ma, IDevice *device) = 0;
            virtual Result GetBatteryCurrent(int *out_ma, IDevice *device) = 0;

            virtual Result GetChargerInputCurrentLimit(int *out_ma, IDevice *device) = 0;
            virtual Result SetChargerInputCurrentLimit(IDevice *device, int ma) = 0;

            virtual Result SetChargerInputVoltageLimit(IDevice *device, int mv) = 0;

            virtual Result SetChargerBoostModeCurrentLimit(IDevice *device, int ma) = 0;

            virtual Result GetBatteryInternalState(void *dst, size_t *out_size, IDevice *device, size_t dst_size) = 0;
            virtual Result SetBatteryInternalState(IDevice *device, const void *src, size_t src_size) = 0;

            virtual Result GetBatteryNeedToRestoreParameters(bool *out, IDevice *device) = 0;
            virtual Result SetBatteryNeedToRestoreParameters(IDevice *device, bool en) = 0;

            virtual Result IsBatteryI2cShutdownEnabled(bool *out, IDevice *device) = 0;
            virtual Result SetBatteryI2cShutdownEnabled(IDevice *device, bool en) = 0;

            virtual Result IsBatteryPresent(bool *out, IDevice *device) = 0;

            virtual Result GetChargerChargerStatus(ChargerStatus *out, IDevice *device) = 0;

            virtual Result GetBatteryCycles(int *out, IDevice *device) = 0;
            virtual Result SetBatteryCycles(IDevice *device, int cycles) = 0;

            virtual Result GetBatteryAge(float *out_percent, IDevice *device) = 0;

            virtual Result GetBatteryTemperature(float *out_c, IDevice *device) = 0;
            virtual Result GetBatteryMaximumTemperature(float *out_c, IDevice *device) = 0;

            virtual Result SetBatteryTemperatureMinimumAlertThreshold(IDevice *device, float c) = 0;
            virtual Result SetBatteryTemperatureMaximumAlertThreshold(IDevice *device, float c) = 0;

            virtual Result GetBatteryVCell(int *out_mv, IDevice *device) = 0;
            virtual Result GetBatteryAverageVCell(int *out_mv, IDevice *device) = 0;

            virtual Result GetBatteryAverageVCellTime(TimeSpan *out, IDevice *device) = 0;

            virtual Result SetBatteryVoltageMinimumAlertThreshold(IDevice *device, int mv) = 0;

            virtual Result GetBatteryOpenCircuitVoltage(int *out_mv, IDevice *device) = 0;

            virtual Result SetBatteryVoltageMaximumAlertThreshold(IDevice *device, int mv) = 0;

            virtual Result IsChargerWatchdogTimerEnabled(bool *out, IDevice *device) = 0;
            virtual Result SetChargerWatchdogTimerEnabled(IDevice *device, bool en) = 0;

            virtual Result SetChargerWatchdogTimerTimeout(IDevice *device, TimeSpan timeout) = 0;
            virtual Result ResetChargerWatchdogTimer(IDevice *device) = 0;

            virtual Result GetChargerBatteryCompensation(int *out_mo, IDevice *device) = 0;
            virtual Result SetChargerBatteryCompensation(IDevice *device, int mo) = 0;

            virtual Result GetChargerVoltageClamp(int *out_mv, IDevice *device) = 0;
            virtual Result SetChargerVoltageClamp(IDevice *device, int mv) = 0;
    };

}
