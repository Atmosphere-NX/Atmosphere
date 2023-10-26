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
#pragma once
#include <stratosphere.hpp>
#include "pm_process_info.hpp"

namespace ams::pm::impl {

    class ProcessTracker {
        NON_COPYABLE(ProcessTracker);
        NON_MOVEABLE(ProcessTracker);
        private:
            os::ThreadType m_thread;
            os::EventType m_request_event;
            os::EventType m_reply_event;
            os::SdkMutex m_mutex;
            ProcessInfo *m_queued_process_info;
            util::Atomic<u32> m_process_count;
        public:
            constexpr ProcessTracker() : m_thread(), m_request_event(), m_reply_event(), m_mutex(), m_queued_process_info(nullptr), m_process_count(0) {
                /* ... */
            }

            void Initialize(void *stack, size_t stack_size);
            void StartThread();

            void QueueEntry(ProcessInfo *process_info);

            u32 GetProcessCount() const {
                return m_process_count;
            }
        private:
            void OnProcessSignaled(ProcessInfo *process_info);
        private:
            static void ThreadFunction(void *_this) {
                static_cast<ProcessTracker *>(_this)->ThreadBody();
            }

            void ThreadBody();
    };

    void CreateProcessEvent();

}
