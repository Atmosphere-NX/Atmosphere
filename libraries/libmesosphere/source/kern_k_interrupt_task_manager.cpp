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

    void KInterruptTaskManager::TaskQueue::Enqueue(KInterruptTask *task) {
        MESOSPHERE_ASSERT(task != m_head);
        MESOSPHERE_ASSERT(task != m_tail);
        MESOSPHERE_AUDIT(task->GetNextTask() == nullptr);

        /* Insert the task into the queue. */
        if (m_tail != nullptr) {
            m_tail->SetNextTask(task);
        } else {
            m_head = task;
        }

        m_tail = task;

        /* Set the next task for auditing. */
        #if defined (MESOSPHERE_BUILD_FOR_AUDITING)
        task->SetNextTask(GetDummyInterruptTask());
        #endif
    }

    void KInterruptTaskManager::TaskQueue::Dequeue() {
        MESOSPHERE_ASSERT(m_head != nullptr);
        MESOSPHERE_ASSERT(m_tail != nullptr);
        MESOSPHERE_AUDIT(m_tail->GetNextTask() == GetDummyInterruptTask());

        /* Pop the task from the front of the queue. */
        KInterruptTask *old_head = m_head;

        if (m_head == m_tail) {
            m_head = nullptr;
            m_tail = nullptr;
        } else {
            m_head = m_head->GetNextTask();
        }

        #if defined (MESOSPHERE_BUILD_FOR_AUDITING)
        old_head->SetNextTask(nullptr);
        #else
        AMS_UNUSED(old_head);
        #endif
    }

    void KInterruptTaskManager::EnqueueTask(KInterruptTask *task) {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());

        /* Enqueue the task and signal the scheduler. */
        m_task_queue.Enqueue(task);
        Kernel::GetScheduler().SetInterruptTaskRunnable();
    }

    void KInterruptTaskManager::DoTasks() {
        /* Execute pending tasks. */
        const s64 start_time = KHardwareTimer::GetTick();
        for (KInterruptTask *task = m_task_queue.GetHead(); task != nullptr; task = m_task_queue.GetHead()) {
            /* Dequeue the task. */
            m_task_queue.Dequeue();

            /* Do the task with interrupts temporarily enabled. */
            {
                KScopedInterruptEnable ei;

                task->DoTask();
            }
        }
        const s64 end_time = KHardwareTimer::GetTick();

        /* Increment the time we've spent executing. */
        m_cpu_time += end_time - start_time;
    }

}
