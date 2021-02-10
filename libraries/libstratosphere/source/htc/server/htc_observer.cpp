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
#include "htc_observer.hpp"

namespace ams::htc::server {

    Observer::Observer(const HtcmiscImpl &misc_impl)
        : m_connect_event(os::EventClearMode_ManualClear, true),
          m_disconnect_event(os::EventClearMode_ManualClear, true),
          m_stop_event(os::EventClearMode_ManualClear),
          m_misc_impl(misc_impl),
          m_thread_running(false),
          m_stopped(false),
          m_connected(false),
          m_is_service_available(false)
    {
        /* Initialize htcs library. */
        AMS_ABORT("htcs::impl::HtcsManagerHolder::AddReference();");

        /* Update our event state. */
        this->UpdateEvent();

        /* Start. */
        R_ABORT_UNLESS(this->Start());
    }

    Result Observer::Start() {
        /* Check that we're not already running. */
        AMS_ASSERT(!m_thread_running);

        /* Create the thread. */
        R_TRY(os::CreateThread(std::addressof(m_observer_thread), ObserverThreadEntry, this, m_observer_thread_stack, sizeof(m_observer_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcObserver)));

        /* Set the thread name pointer. */
        os::SetThreadNamePointer(std::addressof(m_observer_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcObserver));

        /* Mark our thread as running. */
        m_thread_running = true;
        m_stopped        = false;

        /* Start our thread. */
        os::StartThread(std::addressof(m_observer_thread));

        return ResultSuccess();
    }

    void Observer::UpdateEvent() {
        if (m_connected && m_is_service_available) {
            m_disconnect_event.Clear();
            m_connect_event.Signal();
        } else {
            m_connect_event.Clear();
            m_disconnect_event.Signal();
        }
    }

    void Observer::ObserverThreadBody() {
        AMS_ABORT("Observer::ObserverThreadBody");
    }

}
