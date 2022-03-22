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
#include "tio_sd_card_observer.hpp"

namespace ams::tio {

    void SdCardObserver::Initialize(void *thread_stack, size_t thread_stack_size) {
        /* Setup our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_thread), ThreadEntry, this, thread_stack, thread_stack_size, AMS_GET_SYSTEM_THREAD_PRIORITY(TioServer, SdCardObserver)));

        /* Set our thread name pointer. */
        os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(TioServer, SdCardObserver));

        /* Set our initial insertion state. */
        m_inserted = fs::IsSdCardInserted();
    }

    void SdCardObserver::SetCallback(SdCardInsertionCallback callback) {
        /* Check that we don't already have a callback. */
        AMS_ABORT_UNLESS(m_callback == nullptr);

        /* Set our callback. */
        m_callback = callback;
    }

    void SdCardObserver::ThreadFunc() {
        /* Open detection event notifier. */
        std::unique_ptr<fs::IEventNotifier> notifier;
        R_ABORT_UNLESS(fs::OpenSdCardDetectionEventNotifier(std::addressof(notifier)));

        /* Bind the detection event. */
        os::SystemEventType event;
        R_ABORT_UNLESS(notifier->BindEvent(std::addressof(event), os::EventClearMode_AutoClear));

        /* Loop, waiting for insertion events. */
        while (true) {
            /* Wait for an event. */
            os::WaitSystemEvent(std::addressof(event));

            /* Update our insertion state. */
            m_inserted = fs::IsSdCardInserted();

            /* Invoke our callback. */
            if (m_callback) {
                m_callback(m_inserted);
            }
        }
    }

}
