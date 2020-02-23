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
#include <mesosphere.hpp>

namespace ams::kern {

    void KInterruptTaskManager::TaskQueue::Enqueue(KInterruptTask *task) {
        MESOSPHERE_ASSERT(task->GetNextTask() == nullptr);
        MESOSPHERE_ASSERT(task != this->head);
        MESOSPHERE_ASSERT(task != this->tail);

        /* Insert the task into the queue. */
        if (this->tail != nullptr) {
            this->tail->SetNextTask(task);
        } else {
            this->head = task;
        }

        this->tail = task;
    }

    void KInterruptTaskManager::TaskQueue::Dequeue() {
        MESOSPHERE_ASSERT(this->head != nullptr);
        MESOSPHERE_ASSERT(this->tail != nullptr);

        /* Pop the task from the front of the queue. */
        if (this->head == this->tail) {
            this->head = nullptr;
            this->tail = nullptr;
        } else {
            this->head = this->head->GetNextTask();
        }
    }

    void KInterruptTaskManager::ThreadFunction(uintptr_t arg) {
        reinterpret_cast<KInterruptTaskManager *>(arg)->ThreadFunctionImpl();
    }

    void KInterruptTaskManager::ThreadFunctionImpl() {
        MESOSPHERE_ASSERT_THIS();

        while (true) {
            /* Get a task. */
            KInterruptTask *task = nullptr;
            {
                KScopedInterruptDisable di;

                task = this->task_queue.GetHead();
                if (task == nullptr) {
                    this->thread->SetState(KThread::ThreadState_Waiting);
                    continue;
                }

                this->task_queue.Dequeue();
            }

            /* Do the task. */
            task->DoTask();
        }
    }

    void KInterruptTaskManager::Initialize() {
        /* Reserve a thread from the system limit. */
        MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_ThreadCountMax, 1));

        /* Create and initialize the thread. */
        this->thread = KThread::Create();
        MESOSPHERE_ABORT_UNLESS(this->thread != nullptr);
        MESOSPHERE_R_ABORT_UNLESS(KThread::InitializeHighPriorityThread(this->thread, ThreadFunction, reinterpret_cast<uintptr_t>(this)));
        KThread::Register(this->thread);

        /* Run the thread. */
        this->thread->Run();
    }

    void KInterruptTaskManager::EnqueueTask(KInterruptTask *task) {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());

        /* Enqueue the task and signal the scheduler. */
        this->task_queue.Enqueue(task);
        Kernel::GetScheduler().SetInterruptTaskThreadRunnable();
    }

}
