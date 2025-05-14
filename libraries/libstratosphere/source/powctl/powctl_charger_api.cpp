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
#include <stratosphere.hpp>
#include "impl/powctl_i_power_control_driver.hpp"
#include "impl/powctl_device_management.hpp"

namespace ams::powctl {

    namespace {

        impl::SessionImpl &GetOpenSessionImpl(Session &session) {
            /* AMS_ASSERT(session.has_session); */
            auto &impl = GetReference(session.impl_storage);
            AMS_ASSERT(impl.IsOpen());
            return impl;
        }

    }

    Result GetChargerChargeCurrentState(ChargeCurrentState *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetChargerChargeCurrentState(out, std::addressof(device)));
    }

    Result SetChargerChargeCurrentState(Session &session, ChargeCurrentState state) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerChargeCurrentState(std::addressof(device), state));
    }

    Result GetChargerFastChargeCurrentLimit(int *out_ma, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetChargerFastChargeCurrentLimit(out_ma, std::addressof(device)));
    }

    Result SetChargerFastChargeCurrentLimit(Session &session, int ma) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerFastChargeCurrentLimit(std::addressof(device), ma));
    }

    Result GetChargerChargeVoltageLimit(int *out_mv, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetChargerChargeVoltageLimit(out_mv, std::addressof(device)));
    }

    Result SetChargerChargeVoltageLimit(Session &session, int mv) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerChargeVoltageLimit(std::addressof(device), mv));
    }

    Result SetChargerChargerConfiguration(Session &session, ChargerConfiguration cfg) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerChargerConfiguration(std::addressof(device), cfg));
    }

    Result IsChargerHiZEnabled(bool *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().IsChargerHiZEnabled(out, std::addressof(device)));
    }

    Result SetChargerHiZEnabled(Session &session, bool en) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerHiZEnabled(std::addressof(device), en));
    }

    Result GetChargerInputCurrentLimit(int *out_ma, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetChargerInputCurrentLimit(out_ma, std::addressof(device)));
    }

    Result SetChargerInputCurrentLimit(Session &session, int ma) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerInputCurrentLimit(std::addressof(device), ma));
    }

    Result SetChargerInputVoltageLimit(Session &session, int mv) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerInputVoltageLimit(std::addressof(device), mv));
    }

    Result SetChargerBoostModeCurrentLimit(Session &session, int ma) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerBoostModeCurrentLimit(std::addressof(device), ma));
    }

    Result GetChargerChargerStatus(ChargerStatus *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetChargerChargerStatus(out, std::addressof(device)));
    }

    Result IsChargerWatchdogTimerEnabled(bool *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().IsChargerWatchdogTimerEnabled(out, std::addressof(device)));
    }

    Result SetChargerWatchdogTimerEnabled(Session &session, bool en) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerWatchdogTimerEnabled(std::addressof(device), en));
    }

    Result SetChargerWatchdogTimerTimeout(Session &session, TimeSpan timeout) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerWatchdogTimerTimeout(std::addressof(device), timeout));
    }

    Result ResetChargerWatchdogTimer(Session &session);

    Result GetChargerBatteryCompensation(int *out_mo, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetChargerBatteryCompensation(out_mo, std::addressof(device)));
    }

    Result SetChargerBatteryCompensation(Session &session, int mo) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerBatteryCompensation(std::addressof(device), mo));
    }

    Result GetChargerVoltageClamp(int *out_mv, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetChargerVoltageClamp(out_mv, std::addressof(device)));
    }

    Result SetChargerVoltageClamp(Session &session, int mv) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetChargerVoltageClamp(std::addressof(device), mv));
    }

}