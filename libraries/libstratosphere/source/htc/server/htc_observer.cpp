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
#include "htc_observer.hpp"
#include "../../htcs/impl/htcs_manager.hpp"

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
        htcs::impl::HtcsManagerHolder::AddReference();

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
        /* When we're done observing, clear our state. */
        ON_SCOPE_EXIT {
            m_connected            = false;
            m_is_service_available = false;
            this->UpdateEvent();
        };

        /* Get the htcs manager. */
        auto * const htcs_manager = htcs::impl::HtcsManagerHolder::GetHtcsManager();

        /* Get the events we're waiting on. */
        os::EventType * const stop_event = m_stop_event.GetBase();
        os::EventType * const conn_event = m_misc_impl.GetConnectionEvent();
        os::EventType * const htcs_event = htcs_manager->GetServiceAvailabilityEvent();

        /* Loop until we're asked to stop. */
        while (!m_stopped) {
            /* Wait for an event to be signaled. */
            const auto index = os::WaitAny(stop_event, conn_event /*, htcs_event */);
            switch (index) {
                case 0:
                    /* Stop event, just break out of the loop. */
                    os::ClearEvent(stop_event);
                    break;
                case 1:
                    /* Connection event, update our connection status. */
                    os::ClearEvent(conn_event);
                    m_connected = m_misc_impl.IsConnected();
                    break;
                case 2:
                    /* Htcs event, update our service status. */
                    os::ClearEvent(htcs_event);
                    m_is_service_available = htcs_manager->IsServiceAvailable();
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* If the event was our stop event, break. */
            if (index == 0) {
                break;
            }

            /* Update event status. */
            this->UpdateEvent();
        }
    }

}
