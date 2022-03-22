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
#include "htcs_rpc_tasks.hpp"

namespace ams::htcs::impl::rpc {

    namespace {

        constexpr int MaxEventCount = 0x22;

        constinit os::SdkMutex g_event_count_mutex;
        constinit int g_event_count = 0;

    }

    HtcsSignalingTask::HtcsSignalingTask(HtcsTaskType type) : HtcsTask(type), m_is_valid(false) {
        /* Acquire the exclusive right to create an event. */
        std::scoped_lock lk(g_event_count_mutex);

        /* Create an event. */
        if (AMS_LIKELY(g_event_count < MaxEventCount)) {
            /* Make the event. */
            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(m_system_event), os::EventClearMode_ManualClear, true));

            /* Increment the event count. */
            ++g_event_count;

            /* Mark ourselves as valid. */
            m_is_valid = true;
        }
    }

    HtcsSignalingTask::~HtcsSignalingTask() {
        /* If we have an event, we need to destroy it. */
        if (AMS_LIKELY(m_is_valid)) {
            /* Acquire exclusive access to the event count. */
            std::scoped_lock lk(g_event_count_mutex);

            /* Destroy our event. */
            os::DestroySystemEvent(std::addressof(m_system_event));

            /* Decrement the event count. */
            if ((--g_event_count) < 0) {
                g_event_count = 0;
            }
        }
    }

}
