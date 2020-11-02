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
#include "pwm_driver_core.hpp"
#include "pwm_channel_session_impl.hpp"

namespace ams::pwm::driver::impl {

    Result ChannelSessionImpl::Open(IPwmDevice *device, ddsf::AccessMode access_mode) {
        AMS_ASSERT(device != nullptr);

        /* Check if we're the device's first session. */
        const bool first = !device->HasAnyOpenSession();

        /* Open the session. */
        R_TRY(ddsf::OpenSession(device, this, access_mode));
        auto guard = SCOPE_GUARD { ddsf::CloseSession(this); };

        /* If we're the first session, initialize the device. */
        if (first) {
            R_TRY(device->GetDriver().SafeCastTo<IPwmDriver>().InitializeDevice(device));
        }

        /* We're opened. */
        guard.Cancel();
        return ResultSuccess();
    }

    void ChannelSessionImpl::Close() {
        /* If we're not open, do nothing. */
        if (!this->IsOpen()) {
            return;
        }

        /* Get the device. */
        auto &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Close the session. */
        ddsf::CloseSession(this);

        /* If there are no remaining sessions, finalize the device. */
        if (!device.HasAnyOpenSession()) {
            device.GetDriver().SafeCastTo<IPwmDriver>().FinalizeDevice(std::addressof(device));
        }
    }

    Result ChannelSessionImpl::SetPeriod(TimeSpan period) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().SetPeriod(std::addressof(device), period);
    }

    Result ChannelSessionImpl::GetPeriod(TimeSpan *out) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().GetPeriod(out, std::addressof(device));
    }

    Result ChannelSessionImpl::SetDuty(int duty) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().SetDuty(std::addressof(device), duty);
    }

    Result ChannelSessionImpl::GetDuty(int *out) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().GetDuty(out, std::addressof(device));
    }

    Result ChannelSessionImpl::SetEnabled(bool en) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().SetEnabled(std::addressof(device), en);
    }

    Result ChannelSessionImpl::GetEnabled(bool *out) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().GetEnabled(out, std::addressof(device));
    }

    Result ChannelSessionImpl::SetScale(double scale) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().SetScale(std::addressof(device), scale);
    }

    Result ChannelSessionImpl::GetScale(double *out) {
        /* Get the device. */
        IPwmDevice &device = this->GetDevice().SafeCastTo<IPwmDevice>();

        /* Invoke the driver handler. */
        return device.GetDriver().SafeCastTo<IPwmDriver>().GetScale(out, std::addressof(device));
    }

}
