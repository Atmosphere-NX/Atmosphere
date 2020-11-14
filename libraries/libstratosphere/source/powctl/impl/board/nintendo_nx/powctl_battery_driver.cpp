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
#include "../../powctl_device_management.hpp"
#include "powctl_retry_helper.hpp"
#include "powctl_battery_driver.hpp"
#include "powctl_max17050_driver.hpp"

namespace ams::powctl::impl::board::nintendo_nx {

    namespace {

        constinit std::optional<BatteryDevice> g_battery_device;

        Max17050Driver &GetMax17050Driver() {
            static Max17050Driver s_max17050_driver;
            return s_max17050_driver;
        }

        constexpr inline const double SenseResistorValue = 0.005;

    }

    BatteryDevice::BatteryDevice(bool ev) : use_event_handler(ev), event_handler() {
        if (this->use_event_handler) {
            /* Create the system event. */
            os::CreateSystemEvent(std::addressof(this->system_event), os::EventClearMode_ManualClear, true);

            /* Create the handler. */
            this->event_handler.emplace(this);

            /* Register the event handler. */
            powctl::impl::RegisterInterruptHandler(std::addressof(*this->event_handler));
        }
    }

    /* Generic API. */
    void BatteryDriver::InitializeDriver() {
        /* Initialize gpio library. */
        gpio::Initialize();

        /* Create battery device. */
        g_battery_device.emplace(this->IsEventHandlerEnabled());

        /* Initialize the Max17050Driver. */
        {
            size_t battery_vendor_size;
            char battery_vendor[0x18] = {};
            if (R_FAILED(cal::GetBatteryVendor(std::addressof(battery_vendor_size), battery_vendor, sizeof(battery_vendor)))) {
                battery_vendor[7] = 'A';
                battery_vendor_size = 0;
            }

            u8 battery_version = 0;
            if (R_FAILED(cal::GetBatteryVersion(std::addressof(battery_version)))) {
                battery_version = 0;
            }

            GetMax17050Driver().Initialize(battery_vendor, battery_version);
        }

        /* Register our device. */
        this->RegisterDevice(std::addressof(*g_battery_device));

        /* Register the charger device's code. */
        R_ABORT_UNLESS(powctl::impl::RegisterDeviceCode(powctl::DeviceCode_Max17050, std::addressof(*g_battery_device)));

    }

    void BatteryDriver::FinalizeDriver() {
        /* Unregister the charger device code. */
        powctl::impl::UnregisterDeviceCode(powctl::DeviceCode_Max17050);

        /* Unregister our device. */
        this->UnregisterDevice(std::addressof(*g_battery_device));

        /* Finalize Max17050Driver. */
        GetMax17050Driver().Finalize();

        /* Destroy the charger device. */
        g_battery_device = std::nullopt;

        /* Finalize gpio library. */
        gpio::Finalize();
    }

    Result BatteryDriver::GetDeviceSystemEvent(os::SystemEventType **out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Check that we support event handlers. */
        R_UNLESS(this->IsEventHandlerEnabled(), powctl::ResultNotAvailable());

        *out = device->SafeCastTo<BatteryDevice>().GetSystemEvent();
        return ResultSuccess();
    }

    Result BatteryDriver::SetDeviceInterruptEnabled(IDevice *device, bool enable) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Set the interrupt enable. */
        device->SafeCastTo<BatteryDevice>().SetInterruptEnabled(enable);

        return ResultSuccess();
    }

    Result BatteryDriver::GetDeviceErrorStatus(u32 *out, IDevice *device) {
        /* TODO */
        AMS_ABORT();
    }

    Result BatteryDriver::SetDeviceErrorStatus(IDevice *device, u32 status) {
        /* TODO */
        AMS_ABORT();
    }

    Result BatteryDriver::GetBatterySocRep(float *out_percent, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_percent != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr,      powctl::ResultInvalidArgument());

        /* Get the value. */
        double percent;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetSocRep(std::addressof(percent)));

        /* Set output. */
        *out_percent = percent;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatterySocVf(float *out_percent, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_percent != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr,      powctl::ResultInvalidArgument());

        /* Get the value. */
        double percent;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetSocVf(std::addressof(percent)));

        /* Set output. */
        *out_percent = percent;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryFullCapacity(int *out_mah, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mah != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        /* Get the value. */
        double mah;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetFullCapacity(std::addressof(mah), SenseResistorValue));

        /* Set output. */
        *out_mah = mah;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryRemainingCapacity(int *out_mah, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mah != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        /* Get the value. */
        double mah;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetRemainingCapacity(std::addressof(mah), SenseResistorValue));

        /* Set output. */
        *out_mah = mah;
        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryPercentageMinimumAlertThreshold(IDevice *device, float percentage) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetPercentageMinimumAlertThreshold(percentage));

        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryPercentageMaximumAlertThreshold(IDevice *device, float percentage) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetPercentageMaximumAlertThreshold(percentage));

        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryPercentageFullThreshold(IDevice *device, float percentage) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetPercentageFullThreshold(percentage));

        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryAverageCurrent(int *out_ma, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_ma != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        double ma;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetAverageCurrent(std::addressof(ma), SenseResistorValue));

        /* Set output. */
        *out_ma = ma;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryCurrent(int *out_ma, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_ma != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        double ma;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetCurrent(std::addressof(ma), SenseResistorValue));

        /* Set output. */
        *out_ma = ma;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryInternalState(void *dst, size_t *out_size, IDevice *device, size_t dst_size) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,                                                                   powctl::ResultInvalidArgument());
        R_UNLESS(dst != nullptr,                                                                      powctl::ResultInvalidArgument());
        R_UNLESS(out_size != nullptr,                                                                 powctl::ResultInvalidArgument());
        R_UNLESS(dst_size == sizeof(max17050::InternalState),                                         powctl::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(reinterpret_cast<uintptr_t>(dst), alignof(max17050::InternalState)), powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().ReadInternalState());
        GetMax17050Driver().GetInternalState(static_cast<max17050::InternalState *>(dst));

        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryInternalState(IDevice *device, const void *src, size_t src_size) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,                                                                   powctl::ResultInvalidArgument());
        R_UNLESS(src != nullptr,                                                                      powctl::ResultInvalidArgument());
        R_UNLESS(src_size == sizeof(max17050::InternalState),                                         powctl::ResultInvalidArgument());
        R_UNLESS(util::IsAligned(reinterpret_cast<uintptr_t>(src), alignof(max17050::InternalState)), powctl::ResultInvalidArgument());

        GetMax17050Driver().SetInternalState(*static_cast<const max17050::InternalState *>(src));
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().WriteInternalState());

        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryNeedToRestoreParameters(bool *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetNeedToRestoreParameters(out));

        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryNeedToRestoreParameters(IDevice *device, bool en) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Set the value. */
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetNeedToRestoreParameters(en));

        return ResultSuccess();
    }

    Result BatteryDriver::IsBatteryI2cShutdownEnabled(bool *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().IsI2cShutdownEnabled(out));

        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryI2cShutdownEnabled(IDevice *device, bool en) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Set the value. */
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetI2cShutdownEnabled(en));

        return ResultSuccess();
    }

    Result BatteryDriver::IsBatteryPresent(bool *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the battery status. */
        u16 status;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetStatus(std::addressof(status)));

        /* Set output. */
        *out = (status & 0x0008) == 0;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryCycles(int *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the battery cycles. */
        u16 cycles;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetCycles(std::addressof(cycles)));

        /* Set output. */
        *out = cycles;
        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryCycles(IDevice *device, int cycles) {
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(cycles == 0,       powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().ResetCycles());

        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryAge(float *out_percent, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_percent != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr,      powctl::ResultInvalidArgument());

        /* Get the value. */
        double percent;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetAge(std::addressof(percent)));

        /* Set output. */
        *out_percent = percent;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryTemperature(float *out_c, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_c != nullptr,  powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        double temp;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetTemperature(std::addressof(temp)));

        /* Set output. */
        *out_c = temp;
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryMaximumTemperature(float *out_c, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_c != nullptr,  powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        u8 max_temp;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetMaximumTemperature(std::addressof(max_temp)));

        /* Set output. */
        *out_c = static_cast<float>(max_temp);
        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryTemperatureMinimumAlertThreshold(IDevice *device, float c) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetTemperatureMinimumAlertThreshold(c));

        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryTemperatureMaximumAlertThreshold(IDevice *device, float c) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetTemperatureMaximumAlertThreshold(c));

        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryVCell(int *out_mv, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mv != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetVCell(out_mv));

        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryAverageVCell(int *out_mv, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mv != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetAverageVCell(out_mv));

        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryAverageVCellTime(TimeSpan *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        double ms;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetAverageVCellTime(std::addressof(ms)));

        /* Set output. */
        *out = TimeSpan::FromMicroSeconds(static_cast<s64>(ms * 1000.0));
        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryVoltageMinimumAlertThreshold(IDevice *device, int mv) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetVoltageMinimumAlertThreshold(mv));

        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryOpenCircuitVoltage(int *out_mv, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mv != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Get the value. */
        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().GetOpenCircuitVoltage(out_mv));

        return ResultSuccess();
    }

    Result BatteryDriver::SetBatteryVoltageMaximumAlertThreshold(IDevice *device, int mv) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr,  powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetMax17050Driver().SetVoltageMaximumAlertThreshold(mv));

        return ResultSuccess();
    }

}
