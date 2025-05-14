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

    Result GetBatteryChargePercentage(float *out_percent, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryChargePercentage(out_percent, std::addressof(device)));
    }

    Result GetBatteryVoltageFuelGaugePercentage(float *out_percent, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryVoltageFuelGaugePercentage(out_percent, std::addressof(device)));
    }

    Result GetBatteryFullCapacity(int *out_mah, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryFullCapacity(out_mah, std::addressof(device)));
    }

    Result GetBatteryRemainingCapacity(int *out_mah, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryRemainingCapacity(out_mah, std::addressof(device)));
    }

    Result SetBatteryChargePercentageMinimumAlertThreshold(Session &session, float percentage) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryChargePercentageMinimumAlertThreshold(std::addressof(device), percentage));
    }

    Result SetBatteryChargePercentageMaximumAlertThreshold(Session &session, float percentage) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryChargePercentageMaximumAlertThreshold(std::addressof(device), percentage));
    }

    Result SetBatteryVoltageFuelGaugePercentageMinimumAlertThreshold(Session &session, float percentage) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryVoltageFuelGaugePercentageMinimumAlertThreshold(std::addressof(device), percentage));
    }

    Result SetBatteryVoltageFuelGaugePercentageMaximumAlertThreshold(Session &session, float percentage) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryVoltageFuelGaugePercentageMaximumAlertThreshold(std::addressof(device), percentage));
    }

    Result SetBatteryFullChargeThreshold(Session &session, float percentage) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryFullChargeThreshold(std::addressof(device), percentage));
    }

    Result GetBatteryAverageCurrent(int *out_ma, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryAverageCurrent(out_ma, std::addressof(device)));
    }

    Result GetBatteryCurrent(int *out_ma, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryCurrent(out_ma, std::addressof(device)));
    }

    Result GetBatteryInternalState(void *dst, size_t *out_size, Session &session, size_t dst_size) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryInternalState(dst, out_size, std::addressof(device), dst_size));
    }

    Result SetBatteryInternalState(Session &session, const void *src, size_t src_size) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryInternalState(std::addressof(device), src, src_size));
    }

    Result GetBatteryNeedToRestoreParameters(bool *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryNeedToRestoreParameters(out, std::addressof(device)));
    }

    Result SetBatteryNeedToRestoreParameters(Session &session, bool en) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryNeedToRestoreParameters(std::addressof(device), en));
    }

    Result IsBatteryI2cShutdownEnabled(bool *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().IsBatteryI2cShutdownEnabled(out, std::addressof(device)));
    }

    Result SetBatteryI2cShutdownEnabled(Session &session, bool en) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryI2cShutdownEnabled(std::addressof(device), en));
    }

    Result IsBatteryPresent(bool *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().IsBatteryPresent(out, std::addressof(device)));
    }

    Result GetBatteryCycles(int *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryCycles(out, std::addressof(device)));
    }

    Result SetBatteryCycles(Session &session, int cycles) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryCycles(std::addressof(device), cycles));
    }

    Result GetBatteryAge(float *out_percent, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryAge(out_percent, std::addressof(device)));
    }

    Result GetBatteryTemperature(float *out_c, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryTemperature(out_c, std::addressof(device)));
    }

    Result GetBatteryMaximumTemperature(float *out_c, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryMaximumTemperature(out_c, std::addressof(device)));
    }

    Result SetBatteryTemperatureMinimumAlertThreshold(Session &session, float c) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryTemperatureMinimumAlertThreshold(std::addressof(device), c));
    }

    Result SetBatteryTemperatureMaximumAlertThreshold(Session &session, float c) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryTemperatureMaximumAlertThreshold(std::addressof(device), c));
    }

    Result GetBatteryVCell(int *out_mv, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryVCell(out_mv, std::addressof(device)));
    }

    Result GetBatteryAverageVCell(int *out_mv, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryAverageVCell(out_mv, std::addressof(device)));
    }

    Result GetBatteryAverageVCellTime(TimeSpan *out, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryAverageVCellTime(out, std::addressof(device)));
    }

    Result GetBatteryOpenCircuitVoltage(int *out_mv, Session &session) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().GetBatteryOpenCircuitVoltage(out_mv, std::addressof(device)));
    }

    Result SetBatteryVoltageMinimumAlertThreshold(Session &session, int mv) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryVoltageMinimumAlertThreshold(std::addressof(device), mv));
    }

    Result SetBatteryVoltageMaximumAlertThreshold(Session &session, int mv) {
        /* Get the session impl. */
        auto &impl = GetOpenSessionImpl(session);

        /* Check the access mode. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the device. */
        auto &device = impl.GetDevice().SafeCastTo<impl::IDevice>();

        /* Call into the driver. */
        R_RETURN(device.GetDriver().SafeCastTo<impl::IPowerControlDriver>().SetBatteryVoltageMaximumAlertThreshold(std::addressof(device), mv));
    }

}