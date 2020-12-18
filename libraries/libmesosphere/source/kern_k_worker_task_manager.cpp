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

    void KWorkerTaskManager::Initialize(WorkerType wt, s32 priority) {
        /* Set type, other members already initialized in constructor. */
        m_type = wt;

        /* Reserve a thread from the system limit. */
        MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_ThreadCountMax, 1));

        /* Create a new thread. */
        m_thread = KThread::Create();
        MESOSPHERE_ABORT_UNLESS(m_thread != nullptr);

        /* Launch the new thread. */
        MESOSPHERE_R_ABORT_UNLESS(KThread::InitializeKernelThread(m_thread, ThreadFunction, reinterpret_cast<uintptr_t>(this), priority, cpu::NumCores - 1));

        /* Register the new thread. */
        KThread::Register(m_thread);

        /* Run the thread. */
        m_thread->Run();
    }

    void KWorkerTaskManager::AddTask(WorkerType type, KWorkerTask *task) {
        MESOSPHERE_ASSERT(type <= WorkerType_Count);
        Kernel::GetWorkerTaskManager(type).AddTask(task);
    }

    void KWorkerTaskManager::ThreadFunction(uintptr_t arg) {
        reinterpret_cast<KWorkerTaskManager *>(arg)->ThreadFunctionImpl();
    }

    void KWorkerTaskManager::ThreadFunctionImpl() {
        while (true) {
            KWorkerTask *task = nullptr;

            /* Get a worker task. */
            {
                KScopedSchedulerLock sl;
                task = this->GetTask();

                if (task == nullptr) {
                    /* If there's nothing to do, set ourselves as waiting. */
                    m_active = false;
                    m_thread->SetState(KThread::ThreadState_Waiting);
                    continue;
                }

                m_active = true;
            }

            /* Do the task. */
            task->DoWorkerTask();
        }
    }

    KWorkerTask *KWorkerTaskManager::GetTask() {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        KWorkerTask *next = m_head_task;
        if (next) {
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
            if (!m_active) {
                m_thread->SetState(KThread::ThreadState_Runnable);
            }
        }
    }

}
