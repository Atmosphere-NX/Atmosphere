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
#include <mesosphere/kern_k_interrupt_task.hpp>

namespace ams::kern {

    class KThread;

    class KInterruptTaskManager {
        private:
            class TaskQueue {
                private:
                    KInterruptTask *m_head;
                    KInterruptTask *m_tail;
                public:
                    constexpr ALWAYS_INLINE TaskQueue() : m_head(nullptr), m_tail(nullptr) { /* ... */ }

                    constexpr ALWAYS_INLINE KInterruptTask *GetHead() { return m_head; }
                    constexpr ALWAYS_INLINE bool IsEmpty() const { return m_head == nullptr; }
                    constexpr ALWAYS_INLINE void Clear() { m_head = nullptr; m_tail = nullptr; }

                    void Enqueue(KInterruptTask *task);
                    void Dequeue();
            };
        private:
            TaskQueue m_task_queue;
            s64 m_cpu_time;
        public:
            constexpr KInterruptTaskManager() : m_task_queue(), m_cpu_time(0) { /* ... */ }

            constexpr ALWAYS_INLINE s64 GetCpuTime() const { return m_cpu_time; }

            void EnqueueTask(KInterruptTask *task);
            void DoTasks();
    };

}
