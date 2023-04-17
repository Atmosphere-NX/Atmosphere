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
#include <haze.hpp>

namespace haze {

    bool EventReactor::AddConsumer(EventConsumer *consumer, Waiter waiter) {
        HAZE_ASSERT(m_num_wait_objects + 1 <= svc::ArgumentHandleCountMax);

        /* Add to the end of the list. */
        m_consumers[m_num_wait_objects] = consumer;
        m_waiters[m_num_wait_objects]   = waiter;
        m_num_wait_objects++;

        return true;
    }

    void EventReactor::RemoveConsumer(EventConsumer *consumer) {
        s32 output_index = 0;

        /* Remove the consumer. */
        for (s32 input_index = 0; input_index < m_num_wait_objects; input_index++) {
            if (m_consumers[input_index] == consumer) {
                continue;
            }

            if (output_index != input_index) {
                m_consumers[output_index] = m_consumers[input_index];
                m_waiters[output_index]   = m_waiters[input_index];
            }

            output_index++;
        }

        m_num_wait_objects = output_index;
    }

    Result EventReactor::WaitForImpl(s32 *out_arg_waiter, const Waiter *arg_waiters, s32 num_arg_waiters) {
        HAZE_ASSERT(0 < num_arg_waiters && num_arg_waiters <= svc::ArgumentHandleCountMax);
        HAZE_ASSERT(m_num_wait_objects + num_arg_waiters <= svc::ArgumentHandleCountMax);

        while (true) {
            /* Check if we should wait for an event. */
            R_TRY(m_result);

            /* Insert waiters from argument list. */
            for (s32 i = 0; i < num_arg_waiters; i++) {
                m_waiters[i + m_num_wait_objects] = arg_waiters[i];
            }

            s32 idx;
            HAZE_R_ABORT_UNLESS(waitObjects(std::addressof(idx), m_waiters, m_num_wait_objects + num_arg_waiters, svc::WaitInfinite));

            /* If a waiter in the argument list was signaled, return it. */
            if (idx >= m_num_wait_objects) {
                *out_arg_waiter = idx - m_num_wait_objects;
                R_SUCCEED();
            }

            /* Otherwise, process the event as normal. */
            m_consumers[idx]->ProcessEvent();
        }
    }

}
