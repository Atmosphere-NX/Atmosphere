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
#include <mesosphere.hpp>

namespace ams::kern {

    void KThreadQueue::NotifyAvailable(KThread *waiting_thread, KSynchronizationObject *signaled_object, Result wait_result) {
        MESOSPHERE_UNUSED(waiting_thread, signaled_object, wait_result);
        MESOSPHERE_PANIC("KThreadQueue::NotifyAvailable\n");
    }

    void KThreadQueue::EndWait(KThread *waiting_thread, Result wait_result) {
        /* Set the thread's wait result. */
        waiting_thread->SetWaitResult(wait_result);

        /* Set the thread as runnable. */
        waiting_thread->SetState(KThread::ThreadState_Runnable);

        /* Clear the thread's wait queue. */
        waiting_thread->ClearWaitQueue();

        /* Cancel the thread task. */
        if (m_hardware_timer != nullptr) {
            m_hardware_timer->CancelTask(waiting_thread);
        }
    }

    void KThreadQueue::CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) {
        /* Set the thread's wait result. */
        waiting_thread->SetWaitResult(wait_result);

        /* Set the thread as runnable. */
        waiting_thread->SetState(KThread::ThreadState_Runnable);

        /* Clear the thread's wait queue. */
        waiting_thread->ClearWaitQueue();

        /* Cancel the thread task. */
        if (cancel_timer_task && m_hardware_timer != nullptr) {
            m_hardware_timer->CancelTask(waiting_thread);
        }
    }

    void KThreadQueueWithoutEndWait::EndWait(KThread *waiting_thread, Result wait_result) {
        MESOSPHERE_UNUSED(waiting_thread, wait_result);
        MESOSPHERE_PANIC("KThreadQueueWithoutEndWait::EndWait\n");
    }

}
