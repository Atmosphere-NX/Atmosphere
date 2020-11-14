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

#include "impl/pwm_driver_core.hpp"
#include "impl/pwm_channel_session_impl.hpp"

namespace ams::pwm::driver {

    namespace {

        Result OpenSessionImpl(ChannelSession *out, IPwmDevice *device) {
            /* Construct the session. */
            auto *session = new (std::addressof(impl::GetChannelSessionImpl(*out))) impl::ChannelSessionImpl;
            auto session_guard = SCOPE_GUARD { session->~ChannelSessionImpl(); };

            /* Open the session. */
            R_TRY(session->Open(device, ddsf::AccessMode_ReadWrite));

            /* We succeeded. */
            session_guard.Cancel();
            return ResultSuccess();
        }

    }

    Result OpenSession(ChannelSession *out, DeviceCode device_code) {
        AMS_ASSERT(out != nullptr);

        /* Find the device. */
        IPwmDevice *device = nullptr;
        R_TRY(impl::FindDevice(std::addressof(device), device_code));
        AMS_ASSERT(device != nullptr);

        /* Open the session. */
        R_TRY(OpenSessionImpl(out, device));

        return ResultSuccess();
    }

    void CloseSession(ChannelSession &session) {
        impl::GetOpenChannelSessionImpl(session).~ChannelSessionImpl();
    }

    void SetPeriod(ChannelSession &session, TimeSpan period) {
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).SetPeriod(period));
    }

    TimeSpan GetPeriod(ChannelSession &session) {
        TimeSpan out_val;
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).GetPeriod(std::addressof(out_val)));
        return out_val;
    }

    void SetDuty(ChannelSession &session, int duty) {
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).SetDuty(duty));
    }

    int GetDuty(ChannelSession &session) {
        int out_val;
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).GetDuty(std::addressof(out_val)));
        return out_val;
    }

    void SetEnabled(ChannelSession &session, bool en) {
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).SetEnabled(en));
    }

    bool GetEnabled(ChannelSession &session) {
        bool out_val;
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).GetEnabled(std::addressof(out_val)));
        return out_val;
    }

    void SetScale(ChannelSession &session, double scale) {
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).SetScale(scale));
    }

    double GetScale(ChannelSession &session) {
        double out_val;
        R_ABORT_UNLESS(impl::GetOpenChannelSessionImpl(session).GetScale(std::addressof(out_val)));
        return out_val;
    }
}
