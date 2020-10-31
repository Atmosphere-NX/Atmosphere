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
#include "gpio_remote_manager_impl.hpp"

namespace ams::gpio {

    namespace {

        constinit os::SdkMutex g_init_mutex;
        constinit int g_initialize_count = 0;
        constinit bool g_remote = false;
        std::shared_ptr<sf::IManager> g_manager;

        using InternalSession = std::shared_ptr<gpio::sf::IPadSession>;

        InternalSession &GetInterface(GpioPadSession *session) {
            AMS_ASSERT(session->_session != nullptr);
            return *static_cast<InternalSession *>(session->_session);
        }

    }

    void Initialize() {
        std::scoped_lock lk(g_init_mutex);

        if ((g_initialize_count++) == 0) {
            R_ABORT_UNLESS(::gpioInitialize());
            g_manager = ams::sf::MakeShared<sf::IManager, RemoteManagerImpl>();
            g_remote  = true;
        }
    }

    void InitializeWith(std::shared_ptr<gpio::sf::IManager> &&sp) {
        std::scoped_lock lk(g_init_mutex);

        AMS_ABORT_UNLESS(g_initialize_count == 0);

        g_manager = std::move(sp);
        g_initialize_count = 1;
    }

    void Finalize() {
        std::scoped_lock lk(g_init_mutex);

        AMS_ASSERT(g_initialize_count > 0);

        if ((--g_initialize_count) == 0) {
            g_manager.reset();
            if (g_remote) {
                ::gpioExit();
            }
        }
    }

    Result OpenSession(GpioPadSession *out_session, ams::DeviceCode device_code) {
        /* Allocate the session. */
        InternalSession *internal_session = new (std::nothrow) InternalSession;
        AMS_ABORT_UNLESS(internal_session != nullptr);
        auto session_guard = SCOPE_GUARD { delete internal_session; };

        /* Get the session. */
        {
            ams::sf::cmif::ServiceObjectHolder object_holder;
            if (hos::GetVersion() >= hos::Version_7_0_0) {
                R_TRY(g_manager->OpenSession2(std::addressof(object_holder), device_code, ddsf::AccessMode_ReadWrite));
            } else {
                R_TRY(g_manager->OpenSession(std::addressof(object_holder), ConvertToGpioPadName(device_code)));
            }
            *internal_session = object_holder.GetServiceObject<sf::IPadSession>();
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
        delete std::addressof(GetInterface(session));
        session->_session = nullptr;
    }

    Result IsWakeEventActive(bool *out_is_active, ams::DeviceCode device_code) {
        if (hos::GetVersion() >= hos::Version_7_0_0) {
            R_TRY(g_manager->IsWakeEventActive2(out_is_active, device_code));
        } else {
            R_TRY(g_manager->IsWakeEventActive(out_is_active, ConvertToGpioPadName(device_code)));
        }

        return ResultSuccess();
    }

    Direction GetDirection(GpioPadSession *session) {
        Direction out;
        R_ABORT_UNLESS(GetInterface(session)->GetDirection(std::addressof(out)));
        return out;
    }

    void SetDirection(GpioPadSession *session, Direction direction) {
        R_ABORT_UNLESS(GetInterface(session)->SetDirection(direction));
    }

    GpioValue GetValue(GpioPadSession *session) {
        GpioValue out;
        R_ABORT_UNLESS(GetInterface(session)->GetValue(std::addressof(out)));
        return out;
    }

    void SetValue(GpioPadSession *session, GpioValue value) {
        R_ABORT_UNLESS(GetInterface(session)->SetValue(value));
    }

    InterruptMode GetInterruptMode(GpioPadSession *session) {
        InterruptMode out;
        R_ABORT_UNLESS(GetInterface(session)->GetInterruptMode(std::addressof(out)));
        return out;
    }

    void SetInterruptMode(GpioPadSession *session, InterruptMode mode) {
        R_ABORT_UNLESS(GetInterface(session)->SetInterruptMode(mode));
    }

    bool GetInterruptEnable(GpioPadSession *session) {
        bool out;
        R_ABORT_UNLESS(GetInterface(session)->GetInterruptEnable(std::addressof(out)));
        return out;
    }

    void SetInterruptEnable(GpioPadSession *session, bool en) {
        R_ABORT_UNLESS(GetInterface(session)->SetInterruptEnable(en));
    }

    InterruptStatus GetInterruptStatus(GpioPadSession *session) {
        InterruptStatus out;
        R_ABORT_UNLESS(GetInterface(session)->GetInterruptStatus(std::addressof(out)));
        return out;
    }

    void ClearInterruptStatus(GpioPadSession *session) {
        R_ABORT_UNLESS(GetInterface(session)->ClearInterruptStatus());
    }

    int GetDebounceTime(GpioPadSession *session) {
        int out;
        R_ABORT_UNLESS(GetInterface(session)->GetDebounceTime(std::addressof(out)));
        return out;
    }

    void SetDebounceTime(GpioPadSession *session, int ms) {
        R_ABORT_UNLESS(GetInterface(session)->SetDebounceTime(ms));
    }

    bool GetDebounceEnabled(GpioPadSession *session) {
        bool out;
        R_ABORT_UNLESS(GetInterface(session)->GetDebounceEnabled(std::addressof(out)));
        return out;
    }

    void SetDebounceEnabled(GpioPadSession *session, bool en) {
        R_ABORT_UNLESS(GetInterface(session)->SetDebounceEnabled(en));
    }

    Result BindInterrupt(os::SystemEventType *event, GpioPadSession *session) {
        AMS_ASSERT(session->_event == nullptr);

        ams::sf::CopyHandle handle;
        R_TRY(GetInterface(session)->BindInterrupt(std::addressof(handle)));

        os::AttachReadableHandleToSystemEvent(event, handle.GetValue(), true, os::EventClearMode_ManualClear);

        session->_event = event;
        return ResultSuccess();
    }

    void UnbindInterrupt(GpioPadSession *session) {
        AMS_ASSERT(session->_event != nullptr);

        R_ABORT_UNLESS(GetInterface(session)->UnbindInterrupt());

        os::DestroySystemEvent(session->_event);
        session->_event = nullptr;
    }

}
