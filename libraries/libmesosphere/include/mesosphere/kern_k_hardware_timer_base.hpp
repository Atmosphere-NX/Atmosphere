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
#pragma once
#include <mesosphere/kern_k_spin_lock.hpp>
#include <mesosphere/kern_k_timer_task.hpp>
#include <mesosphere/kern_select_interrupt_manager.hpp>

namespace ams::kern {

    class KHardwareTimerBase {
        private:
            using TimerTaskTree = util::IntrusiveRedBlackTreeBaseTraits<KTimerTask>::TreeType<KTimerTask>;
        private:
            KSpinLock m_lock;
            TimerTaskTree m_task_tree;
            KTimerTask *m_next_task;
        public:
            constexpr ALWAYS_INLINE KHardwareTimerBase() : m_lock(), m_task_tree(), m_next_task(nullptr) { /* ... */ }
        private:
            ALWAYS_INLINE void RemoveTaskFromTree(KTimerTask *task) {
                /* Erase from the tree. */
                auto it = m_task_tree.erase(m_task_tree.iterator_to(*task));

                /* Clear the task's scheduled time. */
                task->SetTime(0);

                /* Update our next task if relevant. */
                if (m_next_task == task) {
                    m_next_task = (it != m_task_tree.end()) ? std::addressof(*it) : nullptr;
                }
            }
        public:
            NOINLINE void CancelTask(KTimerTask *task) {
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(m_lock);

                if (const s64 task_time = task->GetTime(); task_time > 0) {
                    this->RemoveTaskFromTree(task);
                }
            }
        protected:
            ALWAYS_INLINE KSpinLock &GetLock() { return m_lock; }

            ALWAYS_INLINE s64 DoInterruptTaskImpl(s64 cur_time) {
                /* We want to handle all tasks, returning the next time that a task is scheduled. */
                while (true) {
                    /* Get the next task. If there isn't one, return 0. */
                    KTimerTask *task = m_next_task;
                    if (task == nullptr) {
                        return 0;
                    }

                    /* If the task needs to be done in the future, do it in the future and not now. */
                    if (const s64 task_time = task->GetTime(); task_time > cur_time) {
                        return task_time;
                    }

                    /* Remove the task from the tree of tasks, and update our next task. */
                    this->RemoveTaskFromTree(task);

                    /* Handle the task. */
                    task->OnTimer();
                }
            }

            ALWAYS_INLINE bool RegisterAbsoluteTaskImpl(KTimerTask *task, s64 task_time) {
                MESOSPHERE_ASSERT(task_time > 0);

                /* Set the task's time, and insert it into our tree. */
                task->SetTime(task_time);
                m_task_tree.insert(*task);

                /* Update our next task if relevant. */
                if (m_next_task != nullptr && m_next_task->GetTime() <= task_time) {
                    return false;
                }
                m_next_task = task;
                return true;
            }
    };

}
