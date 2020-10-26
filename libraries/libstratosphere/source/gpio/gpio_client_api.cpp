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

namespace ams::gpio {

    /* TODO: This deserves proper new interface def support. */

    namespace {

        /* TODO: Manager object. */
        constinit os::SdkMutex g_init_mutex;
        constinit int g_initialize_count = 0;

        using InternalSession = ::GpioPadSession;

        InternalSession *GetInterface(GpioPadSession *session) {
            AMS_ASSERT(session->_session != nullptr);
            return static_cast<::GpioPadSession *>(session->_session);
        }

    }

    void Initialize() {
        std::scoped_lock lk(g_init_mutex);

        if ((g_initialize_count++) == 0) {
            R_ABORT_UNLESS(::gpioInitialize());
        }
    }

    void Finalize() {
        std::scoped_lock lk(g_init_mutex);

        AMS_ASSERT(g_initialize_count > 0);

        if ((--g_initialize_count) == 0) {
            ::gpioExit();
        }
    }

    Result OpenSession(GpioPadSession *out_session, ams::DeviceCode device_code) {
        /* Allocate the session. */
        InternalSession *internal_session = static_cast<InternalSession *>(std::malloc(sizeof(InternalSession)));
        AMS_ABORT_UNLESS(internal_session != nullptr);
        auto session_guard = SCOPE_GUARD { std::free(internal_session); };

        /* Get the session. */
        if (hos::GetVersion() >= hos::Version_7_0_0) {
            R_TRY(::gpioOpenSession2(internal_session, device_code.GetInternalValue(), ddsf::AccessMode_ReadWrite));
        } else {
            R_ABORT_UNLESS(::gpioOpenSession(internal_session, static_cast<::GpioPadName>(static_cast<s32>(ConvertToGpioPadName(device_code)))));
        }

        /* Set output. */
        out_session->_session = internal_session;
        out_session->_event   = nullptr;

        /* We succeeded. */
        session_guard.Cancel();
        return ResultSuccess();
    }

    void CloseSession(GpioPadSession *session) {
        AMS_ASSERT(session != nullptr);

        /* Unbind the interrupt, if it's still bound. */
        if (session->_event != nullptr) {
            gpio::UnbindInterrupt(session);
        }

        /* Close the session. */
        ::gpioPadClose(GetInterface(session));
        std::free(session->_session);
        session->_session = nullptr;
    }

    Result IsWakeEventActive(bool *out_is_active, ams::DeviceCode device_code) {
        if (hos::GetVersion() >= hos::Version_7_0_0) {
            R_TRY(::gpioIsWakeEventActive2(out_is_active, device_code.GetInternalValue()));
        } else {
            R_ABORT_UNLESS(::gpioIsWakeEventActive(out_is_active, static_cast<::GpioPadName>(static_cast<s32>(ConvertToGpioPadName(device_code)))));
        }

        return ResultSuccess();
    }

    Direction GetDirection(GpioPadSession *session) {
        ::GpioDirection out;
        R_ABORT_UNLESS(::gpioPadGetDirection(GetInterface(session), std::addressof(out)));
        return static_cast<Direction>(static_cast<s32>(out));
    }

    void SetDirection(GpioPadSession *session, Direction direction) {
        R_ABORT_UNLESS(::gpioPadSetDirection(GetInterface(session), static_cast<::GpioDirection>(static_cast<s32>(direction))));
    }

    GpioValue GetValue(GpioPadSession *session) {
        ::GpioValue out;
        R_ABORT_UNLESS(::gpioPadGetValue(GetInterface(session), std::addressof(out)));
        return static_cast<GpioValue>(static_cast<s32>(out));
    }

    void SetValue(GpioPadSession *session, GpioValue value) {
        R_ABORT_UNLESS(::gpioPadSetValue(GetInterface(session), static_cast<::GpioValue>(static_cast<s32>(value))));
    }

    InterruptMode GetInterruptMode(GpioPadSession *session) {
        ::GpioInterruptMode out;
        R_ABORT_UNLESS(::gpioPadGetInterruptMode(GetInterface(session), std::addressof(out)));
        return static_cast<InterruptMode>(static_cast<s32>(out));
    }

    void SetInterruptMode(GpioPadSession *session, InterruptMode mode) {
        R_ABORT_UNLESS(::gpioPadSetInterruptMode(GetInterface(session), static_cast<::GpioInterruptMode>(static_cast<s32>(mode))));
    }

    bool GetInterruptEnable(GpioPadSession *session) {
        bool out;
        R_ABORT_UNLESS(::gpioPadGetInterruptEnable(GetInterface(session), std::addressof(out)));
        return out;
    }

    void SetInterruptEnable(GpioPadSession *session, bool en) {
        R_ABORT_UNLESS(::gpioPadSetInterruptEnable(GetInterface(session), en));
    }

    InterruptStatus GetInterruptStatus(GpioPadSession *session) {
        ::GpioInterruptStatus out;
        R_ABORT_UNLESS(::gpioPadGetInterruptStatus(GetInterface(session), std::addressof(out)));
        return static_cast<InterruptStatus>(static_cast<s32>(out));
    }

    void ClearInterruptStatus(GpioPadSession *session) {
        R_ABORT_UNLESS(::gpioPadClearInterruptStatus(GetInterface(session)));
    }

    int GetDebounceTime(GpioPadSession *session) {
        int out;
        R_ABORT_UNLESS(::gpioPadGetDebounceTime(GetInterface(session), std::addressof(out)));
        return out;
    }

    void SetDebounceTime(GpioPadSession *session, int ms) {
        R_ABORT_UNLESS(::gpioPadSetDebounceTime(GetInterface(session), ms));
    }

    bool GetDebounceEnabled(GpioPadSession *session) {
        bool out;
        R_ABORT_UNLESS(::gpioPadGetDebounceEnabled(GetInterface(session), std::addressof(out)));
        return out;
    }

    void SetDebounceEnabled(GpioPadSession *session, bool en) {
        R_ABORT_UNLESS(::gpioPadSetDebounceEnabled(GetInterface(session), en));
    }

    Result BindInterrupt(os::SystemEventType *event, GpioPadSession *session) {
        ::Event ev;
        R_TRY(::gpioPadBindInterrupt(GetInterface(session), std::addressof(ev)));

        os::AttachReadableHandleToSystemEvent(event, ev.revent, true, os::EventClearMode_ManualClear);

        session->_event = event;
        return ResultSuccess();
    }

    void UnbindInterrupt(GpioPadSession *session) {
        AMS_ASSERT(session->_event != nullptr);

        R_ABORT_UNLESS(::gpioPadUnbindInterrupt(GetInterface(session)));

        os::DestroySystemEvent(session->_event);
        session->_event = nullptr;
    }

}
