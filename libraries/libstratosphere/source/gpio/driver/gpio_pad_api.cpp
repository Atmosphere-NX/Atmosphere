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

#include "impl/gpio_driver_core.hpp"

namespace ams::gpio::driver {

    namespace {

        Result OpenSessionImpl(GpioPadSession *out, Pad *pad, ddsf::AccessMode access_mode) {
            /* Construct the session. */
            auto *session = new (std::addressof(impl::GetPadSessionImpl(*out))) impl::PadSessionImpl;
            auto session_guard = SCOPE_GUARD { session->~PadSessionImpl(); };

            /* Open the session. */
            R_TRY(session->Open(pad, access_mode));

            session_guard.Cancel();
            return ResultSuccess();
        }

    }

    Result OpenSession(GpioPadSession *out, DeviceCode device_code, ddsf::AccessMode access_mode) {
        /* Find the pad. */
        Pad *pad = nullptr;
        R_TRY(impl::FindPad(std::addressof(pad), device_code));
        AMS_ASSERT(pad != nullptr);

        /* Sanitize the access mode. */
        if ((access_mode & ~(ddsf::AccessMode_ReadWrite)) != 0) {
            access_mode = ddsf::AccessMode_None;
        }

        /* Try to open the session with the desired mode. */
        R_TRY_CATCH(OpenSessionImpl(out, pad, access_mode)) {
            R_CATCH(ddsf::ResultAccessModeDenied) {
                /* Our current access mode was denied. Try adding the shared flag. */
                R_TRY(OpenSessionImpl(out, pad, static_cast<ddsf::AccessMode>(access_mode | ddsf::AccessMode_Shared)));
            }
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    void CloseSession(GpioPadSession *session) {
        AMS_ASSERT(session != nullptr);
        impl::GetOpenPadSessionImpl(*session).~PadSessionImpl();
    }

    Result SetDirection(GpioPadSession *session, gpio::Direction direction) {
        /* Check that we have a valid session. */
        AMS_ASSERT(session != nullptr);
        auto &impl = impl::GetOpenPadSessionImpl(*session);
        R_UNLESS(impl.IsOpen(), gpio::ResultNotOpen());

        /* Check that we can perform the access. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the pad. */
        auto &pad = impl.GetDevice().SafeCastTo<Pad>();

        /* Perform the call. */
        R_TRY(pad.GetDriver().SafeCastTo<IGpioDriver>().SetDirection(std::addressof(pad), direction));

        return ResultSuccess();
    }

    Result GetDirection(gpio::Direction *out, GpioPadSession *session) {
        /* Check that we have a valid session. */
        AMS_ASSERT(session != nullptr);
        auto &impl = impl::GetOpenPadSessionImpl(*session);
        R_UNLESS(impl.IsOpen(), gpio::ResultNotOpen());

        /* Check that we can perform the access. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the pad. */
        auto &pad = impl.GetDevice().SafeCastTo<Pad>();

        /* Perform the call. */
        R_TRY(pad.GetDriver().SafeCastTo<IGpioDriver>().GetDirection(out, std::addressof(pad)));

        return ResultSuccess();
    }

    Result SetValue(GpioPadSession *session, gpio::GpioValue value) {
        /* Check that we have a valid session. */
        AMS_ASSERT(session != nullptr);
        auto &impl = impl::GetOpenPadSessionImpl(*session);
        R_UNLESS(impl.IsOpen(), gpio::ResultNotOpen());

        /* Check that we can perform the access. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Write), ddsf::ResultPermissionDenied());

        /* Get the pad. */
        auto &pad = impl.GetDevice().SafeCastTo<Pad>();

        /* Perform the call. */
        R_TRY(pad.GetDriver().SafeCastTo<IGpioDriver>().SetValue(std::addressof(pad), value));

        return ResultSuccess();
    }

    Result GetValue(gpio::GpioValue *out, GpioPadSession *session) {
        /* Check that we have a valid session. */
        AMS_ASSERT(session != nullptr);
        auto &impl = impl::GetOpenPadSessionImpl(*session);
        R_UNLESS(impl.IsOpen(), gpio::ResultNotOpen());

        /* Check that we can perform the access. */
        R_UNLESS(impl.CheckAccess(ddsf::AccessMode_Read), ddsf::ResultPermissionDenied());

        /* Get the pad. */
        auto &pad = impl.GetDevice().SafeCastTo<Pad>();

        /* Perform the call. */
        R_TRY(pad.GetDriver().SafeCastTo<IGpioDriver>().GetValue(out, std::addressof(pad)));

        return ResultSuccess();
    }

}
