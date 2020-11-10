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
#include "powctl_charger_driver.hpp"
#include "powctl_bq24193_driver.hpp"

namespace ams::powctl::impl::board::nintendo_nx {

    namespace {

        constinit std::optional<ChargerDevice> g_charger_device;

        Bq24193Driver &GetBq24193Driver() {
            static Bq24193Driver s_bq24193_driver;
            return s_bq24193_driver;
        }

    }

    ChargerDevice::ChargerDevice(bool ev) : gpio_pad_session(), watchdog_timer_enabled(false), watchdog_timer_timeout(0), use_event_handler(ev), event_handler() {
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
    void ChargerDriver::InitializeDriver() {
        /* Initialize Bq24193Driver */
        GetBq24193Driver().Initialize();

        /* Initialize gpio library. */
        gpio::Initialize();

        /* Create charger device. */
        g_charger_device.emplace(this->IsEventHandlerEnabled());

        /* Open the device's gpio session. */
        R_ABORT_UNLESS(gpio::OpenSession(g_charger_device->GetPadSession(), gpio::DeviceCode_BattChgEnableN));

        /* Configure the gpio session as output. */
        gpio::SetDirection(g_charger_device->GetPadSession(), gpio::Direction_Output);

        /* Register our device. */
        this->RegisterDevice(std::addressof(*g_charger_device));

        /* Register the charger device's code. */
        R_ABORT_UNLESS(powctl::impl::RegisterDeviceCode(powctl::DeviceCode_Bq24193, std::addressof(*g_charger_device)));
    }

    void ChargerDriver::FinalizeDriver() {
        /* Unregister the charger device code. */
        powctl::impl::UnregisterDeviceCode(powctl::DeviceCode_Bq24193);

        /* Unregister our device. */
        this->UnregisterDevice(std::addressof(*g_charger_device));

        /* Close the device's gpio session. */
        gpio::CloseSession(g_charger_device->GetPadSession());

        /* Destroy the charger device. */
        g_charger_device = std::nullopt;

        /* Finalize gpio library. */
        gpio::Finalize();

        /* Finalize Bq24193Driver. */
        GetBq24193Driver().Finalize();
    }

    Result ChargerDriver::GetDeviceSystemEvent(os::SystemEventType **out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Check that we support event handlers. */
        R_UNLESS(this->IsEventHandlerEnabled(), powctl::ResultNotAvailable());

        *out = device->SafeCastTo<ChargerDevice>().GetSystemEvent();
        return ResultSuccess();
    }

    Result ChargerDriver::SetDeviceInterruptEnabled(IDevice *device, bool enable) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Set the interrupt enable. */
        device->SafeCastTo<ChargerDevice>().SetInterruptEnabled(enable);

        return ResultSuccess();
    }

    Result ChargerDriver::GetDeviceErrorStatus(u32 *out, IDevice *device) {
        /* TODO */
        AMS_ABORT();
    }

    Result ChargerDriver::SetDeviceErrorStatus(IDevice *device, u32 status) {
        /* TODO */
        AMS_ABORT();
    }

    /* Charger API. */
    Result ChargerDriver::GetChargerChargeCurrentState(ChargeCurrentState *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        /* Check if we're not charging. */
        if (gpio::GetValue(device->SafeCastTo<ChargerDevice>().GetPadSession()) == gpio::GpioValue_High) {
            *out = ChargeCurrentState_NotCharging;
        } else {
            /* Get force 20 percent charge state. */
            bool force_20_percent;
            AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().GetForce20PercentChargeCurrent(std::addressof(force_20_percent)));

            /* Set output appropriately. */
            if (force_20_percent) {
                *out = ChargeCurrentState_ChargingForce20Percent;
            } else {
                *out = ChargeCurrentState_Charging;
            }
        }

        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerChargeCurrentState(IDevice *device, ChargeCurrentState state) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        switch (state) {
            case ChargeCurrentState_NotCharging:
                gpio::SetValue(device->SafeCastTo<ChargerDevice>().GetPadSession(), gpio::GpioValue_High);
                break;
            case ChargeCurrentState_ChargingForce20Percent:
            case ChargeCurrentState_Charging:
                gpio::SetValue(device->SafeCastTo<ChargerDevice>().GetPadSession(), gpio::GpioValue_Low);
                AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetForce20PercentChargeCurrent(state == ChargeCurrentState_ChargingForce20Percent));
                break;
            case ChargeCurrentState_Unknown:
                return powctl::ResultInvalidArgument();
        }

        return ResultSuccess();
    }

    Result ChargerDriver::GetChargerFastChargeCurrentLimit(int *out_ma, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_ma != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().GetFastChargeCurrentLimit(out_ma));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerFastChargeCurrentLimit(IDevice *device, int ma) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetFastChargeCurrentLimit(ma));
        return ResultSuccess();
    }

    Result ChargerDriver::GetChargerChargeVoltageLimit(int *out_mv, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mv != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().GetChargeVoltageLimit(out_mv));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerChargeVoltageLimit(IDevice *device, int mv) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetChargeVoltageLimit(mv));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerChargerConfiguration(IDevice *device, ChargerConfiguration cfg) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        bq24193::ChargerConfiguration bq_cfg;
        switch (cfg) {
            case ChargerConfiguration_ChargeDisable: bq_cfg = bq24193::ChargerConfiguration_ChargeDisable; break;
            case ChargerConfiguration_ChargeBattery: bq_cfg = bq24193::ChargerConfiguration_ChargeBattery; break;
            case ChargerConfiguration_Otg:           bq_cfg = bq24193::ChargerConfiguration_Otg;           break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetChargerConfiguration(bq_cfg));
        return ResultSuccess();
    }

    Result ChargerDriver::IsChargerHiZEnabled(bool *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().IsHiZEnabled(out));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerHiZEnabled(IDevice *device, bool en) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetHiZEnabled(en));
        return ResultSuccess();
    }

    Result ChargerDriver::GetChargerInputCurrentLimit(int *out_ma, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_ma != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().GetInputCurrentLimit(out_ma));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerInputCurrentLimit(IDevice *device, int ma) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetInputCurrentLimit(ma));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerInputVoltageLimit(IDevice *device, int mv) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetInputVoltageLimit(mv));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerBoostModeCurrentLimit(IDevice *device, int ma) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetBoostModeCurrentLimit(ma));
        return ResultSuccess();
    }

    Result ChargerDriver::GetChargerChargerStatus(ChargerStatus *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        bq24193::ChargerStatus bq_status;
        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().GetChargerStatus(std::addressof(bq_status)));

        switch (bq_status) {
            case bq24193::ChargerStatus_NotCharging:
                *out = ChargerStatus_NotCharging;
                break;
            case bq24193::ChargerStatus_PreCharge:
            case bq24193::ChargerStatus_FastCharging:
                *out = ChargerStatus_Charging;
                break;
            case bq24193::ChargerStatus_ChargeTerminationDone:
                *out = ChargerStatus_ChargeTerminationDone;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return ResultSuccess();
    }

    Result ChargerDriver::IsChargerWatchdogTimerEnabled(bool *out, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out != nullptr,    powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        *out = device->SafeCastTo<ChargerDevice>().IsWatchdogTimerEnabled();
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerWatchdogTimerEnabled(IDevice *device, bool en) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        auto &charger_device = device->SafeCastTo<ChargerDevice>();

        if (en) {
            AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().ResetWatchdogTimer());
            AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetWatchdogTimerSetting(charger_device.GetWatchdogTimerTimeout().GetSeconds()));
        } else {
            AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetWatchdogTimerSetting(0));
        }

        charger_device.SetWatchdogTimerEnabled(en);
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerWatchdogTimerTimeout(IDevice *device, TimeSpan timeout) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        device->SafeCastTo<ChargerDevice>().SetWatchdogTimerTimeout(timeout);
        return ResultSuccess();
    }

    Result ChargerDriver::ResetChargerWatchdogTimer(IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().ResetWatchdogTimer());
        return ResultSuccess();
    }

    Result ChargerDriver::GetChargerBatteryCompensation(int *out_mo, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mo != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().GetBatteryCompensation(out_mo));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerBatteryCompensation(IDevice *device, int mo) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetBatteryCompensation(mo));
        return ResultSuccess();
    }

    Result ChargerDriver::GetChargerVoltageClamp(int *out_mv, IDevice *device) {
        /* Validate arguments. */
        R_UNLESS(out_mv != nullptr, powctl::ResultInvalidArgument());
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().GetVoltageClamp(out_mv));
        return ResultSuccess();
    }

    Result ChargerDriver::SetChargerVoltageClamp(IDevice *device, int mv) {
        /* Validate arguments. */
        R_UNLESS(device != nullptr, powctl::ResultInvalidArgument());

        AMS_POWCTL_R_TRY_WITH_RETRY(GetBq24193Driver().SetVoltageClamp(mv));
        return ResultSuccess();
    }

}