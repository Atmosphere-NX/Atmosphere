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
#include <mesosphere/kern_k_interrupt_task.hpp>

namespace ams::kern {

    class KThread;

    class KInterruptTaskManager {
        private:
            class TaskQueue {
                private:
                    KInterruptTask *head;
                    KInterruptTask *tail;
                public:
                    constexpr TaskQueue() : head(nullptr), tail(nullptr) { /* ... */ }

                    ALWAYS_INLINE KInterruptTask *GetHead() { return this->head; }
                    ALWAYS_INLINE bool IsEmpty() const { return this->head == nullptr; }
                    ALWAYS_INLINE void Clear() { this->head = nullptr; this->tail = nullptr; }

                    void Enqueue(KInterruptTask *task);
                    void Dequeue();
            };
        private:
            TaskQueue task_queue;
            KThread *thread;
        public:
            /* TODO: Actually implement KInterruptTaskManager. This is a placeholder. */
    };

}
