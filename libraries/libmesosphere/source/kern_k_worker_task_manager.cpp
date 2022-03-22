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

    namespace {

        class ThreadQueueImplForKWorkerTaskManager final : public KThreadQueue {
            private:
                KThread **m_waiting_thread;
            public:
                constexpr ThreadQueueImplForKWorkerTaskManager(KThread **t) : KThreadQueue(), m_waiting_thread(t) { /* ... */ }

                virtual void EndWait(KThread *waiting_thread, Result wait_result) override {
                    /* Clear our waiting thread. */
                    *m_waiting_thread = nullptr;

                    /* Invoke the base end wait handler. */
                    KThreadQueue::EndWait(waiting_thread, wait_result);
                }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    MESOSPHERE_UNUSED(waiting_thread, wait_result, cancel_timer_task);
                    MESOSPHERE_PANIC("ThreadQueueImplForKWorkerTaskManager::CancelWait\n");
                }
        };

    }

    void KWorkerTask::DoWorkerTask() {
        if (auto * const thread = this->DynamicCast<KThread *>(); thread != nullptr) {
            return thread->DoWorkerTaskImpl();
        } else {
            auto * const process = this->DynamicCast<KProcess *>();
            MESOSPHERE_ABORT_UNLESS(process != nullptr);

            return process->DoWorkerTaskImpl();
        }
    }

    void KWorkerTaskManager::Initialize(s32 priority) {
        /* Reserve a thread from the system limit. */
        MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_ThreadCountMax, 1));

        /* Create a new thread. */
        KThread *thread = KThread::Create();
        MESOSPHERE_ABORT_UNLESS(thread != nullptr);

        /* Launch the new thread. */
        MESOSPHERE_R_ABORT_UNLESS(KThread::InitializeKernelThread(thread, ThreadFunction, reinterpret_cast<uintptr_t>(this), priority, cpu::NumCores - 1));

        /* Register the new thread. */
        KThread::Register(thread);

        /* Run the thread. */
        thread->Run();
    }

    void KWorkerTaskManager::AddTask(WorkerType type, KWorkerTask *task) {
        MESOSPHERE_ASSERT(type <= WorkerType_Count);
        Kernel::GetWorkerTaskManager(type).AddTask(task);
    }

    void KWorkerTaskManager::ThreadFunction(uintptr_t arg) {
        reinterpret_cast<KWorkerTaskManager *>(arg)->ThreadFunctionImpl();
    }

    void KWorkerTaskManager::ThreadFunctionImpl() {
        /* Create wait queue. */
        ThreadQueueImplForKWorkerTaskManager wait_queue(std::addressof(m_waiting_thread));

        while (true) {
            KWorkerTask *task;

            /* Get a worker task. */
            {
                KScopedSchedulerLock sl;

                task = this->GetTask();

                if (task == nullptr) {
                    /* Wait to have a task. */
                    m_waiting_thread = GetCurrentThreadPointer();
                    GetCurrentThread().BeginWait(std::addressof(wait_queue));
                    continue;
                }
            }

            /* Do the task. */
            task->DoWorkerTask();

            /* Destroy any objects we may need to close. */
            GetCurrentThread().DestroyClosedObjects();
        }
    }

    KWorkerTask *KWorkerTaskManager::GetTask() {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        KWorkerTask *next = m_head_task;

        if (next != nullptr) {
            /* Advance the list. */
            if (m_head_task == m_tail_task) {
                m_head_task = nullptr;
                m_tail_task = nullptr;
            } else {
                m_head_task = m_head_task->GetNextTask();
            }

            /* Clear the next task's next. */
            next->SetNextTask(nullptr);
        }

        return next;
    }

    void KWorkerTaskManager::AddTask(KWorkerTask *task) {
        KScopedSchedulerLock sl;
        MESOSPHERE_ASSERT(task->GetNextTask() == nullptr);

        /* Insert the task. */
        if (m_tail_task) {
            m_tail_task->SetNextTask(task);
            m_tail_task = task;
        } else {
            m_head_task = task;
            m_tail_task = task;

            /* Make ourselves active if we need to. */
            if (m_waiting_thread != nullptr) {
                m_waiting_thread->EndWait(ResultSuccess());
            }
        }
    }

}
