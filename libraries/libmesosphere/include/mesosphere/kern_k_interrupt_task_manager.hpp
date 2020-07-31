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

                    constexpr KInterruptTask *GetHead() { return this->head; }
                    constexpr bool IsEmpty() const { return this->head == nullptr; }
                    constexpr void Clear() { this->head = nullptr; this->tail = nullptr; }

                    void Enqueue(KInterruptTask *task);
                    void Dequeue();
            };
        private:
            TaskQueue task_queue;
            KThread *thread;
        private:
            static void ThreadFunction(uintptr_t arg);
            void ThreadFunctionImpl();
        public:
            constexpr KInterruptTaskManager() : task_queue(), thread(nullptr) { /* ... */ }

            constexpr KThread *GetThread() const { return this->thread; }

            NOINLINE void Initialize();
            void EnqueueTask(KInterruptTask *task);
    };

}
