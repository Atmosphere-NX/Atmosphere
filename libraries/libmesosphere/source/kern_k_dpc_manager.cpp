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

    namespace {

        class KDpcTask {
            private:
                static constinit inline KLightLock s_req_lock;
                static constinit inline KLightLock s_lock;
                static constinit inline KLightConditionVariable s_cond_var;
                static constinit inline u64 s_core_mask;
                static constinit inline KDpcTask *s_task;
            private:
                static bool HasRequest(s32 core_id) {
                    return (s_core_mask & (1ull << core_id)) != 0;
                }

                static void SetRequest(s32 core_id) {
                    s_core_mask |= (1ull << core_id);
                }

                static void ClearRequest(s32 core_id) {
                    s_core_mask &= ~(1ull << core_id);
                }
            public:
                virtual void DoTask() { /* ... */ }

                static void Request(KDpcTask *task) {
                    KScopedLightLock rlk(s_req_lock);

                    /* Acquire the requested task. */
                    MESOSPHERE_ABORT_UNLESS(s_task == nullptr);
                    s_task = task;
                    {
                        KScopedLightLock lk(s_lock);
                        MESOSPHERE_ABORT_UNLESS(s_core_mask == 0);

                        for (auto core = 0; core < static_cast<s32>(cpu::NumCores); ++core) {
                            SetRequest(core);
                        }

                        s_cond_var.Broadcast();

                        while (s_core_mask != 0) {
                            s_cond_var.Wait(std::addressof(s_lock), -1ll);
                        }
                    }
                    s_task = nullptr;
                }

                static void WaitForRequest() {
                    /* Wait for a request to come in. */
                    const auto core_id = GetCurrentCoreId();
                    KScopedLightLock lk(s_lock);
                    while (!HasRequest(core_id)) {
                        s_cond_var.Wait(std::addressof(s_lock), -1ll);
                    }
                }

                static bool TimedWaitForRequest(s64 timeout) {
                    /* Wait for a request to come in. */
                    const auto core_id = GetCurrentCoreId();
                    KScopedLightLock lk(s_lock);
                    while (!HasRequest(core_id)) {
                        s_cond_var.Wait(std::addressof(s_lock), timeout);
                        if (KHardwareTimer::GetTick() >= timeout) {
                            return false;
                        }
                    }
                    return true;
                }

                static void HandleRequest() {
                    /* Perform the request. */
                    s_task->DoTask();

                    /* Clear the request. */
                    const auto core_id = GetCurrentCoreId();
                    KScopedLightLock lk(s_lock);
                    ClearRequest(core_id);
                    if (s_core_mask == 0) {
                        s_cond_var.Broadcast();
                    }
                }

        };

        /* Convenience definitions. */
        constexpr s32 DpcManagerThreadPriority = 3;
        constexpr s64 DpcManagerTimeout = ams::svc::Tick(TimeSpan::FromMilliSeconds(10));

        /* Globals. */
        s64 g_preemption_priorities[cpu::NumCores];

        /* Manager thread functions. */
        void DpcManagerNormalThreadFunction(uintptr_t arg) {
            /* Input argument goes unused. */
            MESOSPHERE_UNUSED(arg);

            /* Forever wait and service requests. */
            while (true) {
                KDpcTask::WaitForRequest();
                KDpcTask::HandleRequest();
            }
        }

        void DpcManagerPreemptionThreadFunction(uintptr_t arg) {
            /* Input argument goes unused. */
            MESOSPHERE_UNUSED(arg);

            /* Forever wait and service requests, rotating the scheduled queue every 10 ms. */
            s64 timeout = KHardwareTimer::GetTick() + DpcManagerTimeout;
            while (true) {
                if (KDpcTask::TimedWaitForRequest(timeout)) {
                    KDpcTask::HandleRequest();
                } else {
                    /* Rotate the scheduler queue for each core. */
                    KScopedSchedulerLock lk;

                    for (size_t core_id = 0; core_id < cpu::NumCores; core_id++) {
                        if (const s32 priority = g_preemption_priorities[core_id]; priority > DpcManagerThreadPriority) {
                            KScheduler::RotateScheduledQueue(static_cast<s32>(core_id), priority);
                        }
                    }

                    /* Update our next timeout. */
                    timeout = KHardwareTimer::GetTick() + DpcManagerTimeout;
                }
            }
        }

    }

    void KDpcManager::Initialize(s32 core_id, s32 priority) {
        /* Reserve a thread from the system limit. */
        MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_ThreadCountMax, 1));

        /* Create a new thread. */
        KThread *new_thread = KThread::Create();
        MESOSPHERE_ABORT_UNLESS(new_thread != nullptr);

        /* Launch the new thread. */
        g_preemption_priorities[core_id] = priority;
        if (core_id == cpu::NumCores - 1) {
            MESOSPHERE_R_ABORT_UNLESS(KThread::InitializeKernelThread(new_thread, DpcManagerPreemptionThreadFunction, 0, DpcManagerThreadPriority, core_id));
        } else {
            MESOSPHERE_R_ABORT_UNLESS(KThread::InitializeKernelThread(new_thread, DpcManagerNormalThreadFunction,     0, DpcManagerThreadPriority, core_id));
        }

        /* Register the new thread. */
        KThread::Register(new_thread);

        /* Run the thread. */
        new_thread->Run();
    }

    void KDpcManager::HandleDpc() {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());
        MESOSPHERE_ASSERT(!KScheduler::IsSchedulerLockedByCurrentThread());

        /* The only deferred procedure supported by Horizon is thread termination. */
        /* Check if we need to terminate the current thread. */
        KThread *cur_thread = GetCurrentThreadPointer();
        if (cur_thread->IsTerminationRequested()) {
            KScopedInterruptEnable ei;
            cur_thread->Exit();
        }
    }

    void KDpcManager::Sync() {
        MESOSPHERE_ASSERT(!KScheduler::IsSchedulerLockedByCurrentThread());

        KDpcTask dummy_task;
        KDpcTask::Request(std::addressof(dummy_task));
    }

}
