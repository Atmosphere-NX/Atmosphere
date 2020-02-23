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

    bool KScheduler::s_scheduler_update_needed;
    KScheduler::LockType KScheduler::s_scheduler_lock;
    KSchedulerPriorityQueue KScheduler::s_priority_queue;

    namespace {

        class KSchedulerInterruptTask : public KInterruptTask {
            public:
                constexpr KSchedulerInterruptTask() : KInterruptTask() { /* ... */ }

                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    return GetDummyInterruptTask();
                }

                virtual void DoTask() override {
                    MESOSPHERE_PANIC("KSchedulerInterruptTask::DoTask was called!");
                }
        };

        ALWAYS_INLINE void IncrementScheduledCount(KThread *thread) {
            if (KProcess *parent = thread->GetOwnerProcess(); parent != nullptr) {
                parent->IncrementScheduledCount();
            }
        }

        KSchedulerInterruptTask g_scheduler_interrupt_task;

        ALWAYS_INLINE auto *GetSchedulerInterruptTask() {
            return std::addressof(g_scheduler_interrupt_task);
        }

    }

    void KScheduler::Initialize(KThread *idle_thread) {
        /* Set core ID and idle thread. */
        this->core_id = GetCurrentCoreId();
        this->idle_thread = idle_thread;
        this->state.idle_thread_stack = this->idle_thread->GetStackTop();

        /* Insert the main thread into the priority queue. */
        {
            KScopedSchedulerLock lk;
            GetPriorityQueue().PushBack(GetCurrentThreadPointer());
            SetSchedulerUpdateNeeded();
        }

        /* Bind interrupt handler. */
        Kernel::GetInterruptManager().BindHandler(GetSchedulerInterruptTask(), KInterruptName_Scheduler, this->core_id, KInterruptController::PriorityLevel_Scheduler, false, false);
    }

    void KScheduler::Activate() {
        MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() == 1);

        this->state.should_count_idle = KTargetSystem::IsDebugMode();
        this->is_active = true;
        RescheduleCurrentCore();
    }

    void KScheduler::RescheduleOtherCores(u64 cores_needing_scheduling) {
        if (const u64 core_mask = cores_needing_scheduling & ~(1ul << this->core_id); core_mask != 0) {
            cpu::DataSynchronizationBarrier();
            Kernel::GetInterruptManager().SendInterProcessorInterrupt(KInterruptName_Scheduler, core_mask);
        }
    }

    u64 KScheduler::UpdateHighestPriorityThread(KThread *highest_thread) {
        if (KThread *prev_highest_thread = this->state.highest_priority_thread; AMS_LIKELY(prev_highest_thread != highest_thread)) {
            if (AMS_LIKELY(prev_highest_thread != nullptr)) {
                IncrementScheduledCount(prev_highest_thread);
                prev_highest_thread->SetLastScheduledTick(KHardwareTimer::GetTick());
            }
            if (this->state.should_count_idle) {
                if (AMS_LIKELY(highest_thread != nullptr)) {
                    /* TODO: Set parent process's idle count if it exists. */
                } else {
                    this->state.idle_count++;
                }
            }

            this->state.highest_priority_thread = highest_thread;
            this->state.needs_scheduling = true;
            return (1ul << this->core_id);
        } else {
            return 0;
        }
    }

    u64 KScheduler::UpdateHighestPriorityThreadsImpl() {
        MESOSPHERE_ASSERT(IsSchedulerLockedByCurrentThread());

        /* Clear that we need to update. */
        ClearSchedulerUpdateNeeded();

        u64 cores_needing_scheduling = 0, idle_cores = 0;
        KThread *top_threads[cpu::NumCores];
        auto &priority_queue = GetPriorityQueue();

        /* We want to go over all cores, finding the highest priority thread and determining if scheduling is needed for that core. */
        for (size_t core_id = 0; core_id < cpu::NumCores; core_id++) {
            KThread *top_thread = priority_queue.GetScheduledFront(core_id);
            if (top_thread != nullptr) {
                /* If the thread has no waiters, we need to check if the process has a thread pinned by PreemptionState. */
                if (top_thread->GetNumKernelWaiters() == 0) {
                    if (KProcess *parent = top_thread->GetOwnerProcess(); parent != nullptr) {
                        if (KThread *suggested = parent->GetPreemptionStatePinnedThread(core_id); suggested != nullptr && suggested != top_thread) {
                            /* We prefer our parent's pinned thread possible. However, we also don't want to schedule un-runnable threads. */
                            if (suggested->GetRawState() == KThread::ThreadState_Runnable) {
                                top_thread = suggested;
                            } else {
                                top_thread = nullptr;
                            }
                        }
                    }
                }
            } else {
                idle_cores |= (1ul << core_id);
            }

            top_threads[core_id] = top_thread;
            cores_needing_scheduling |= Kernel::GetScheduler(core_id).UpdateHighestPriorityThread(top_threads[core_id]);
        }

        /* Idle cores are bad. We're going to try to migrate threads to each idle core in turn. */
        while (idle_cores != 0) {
            s32 core_id = __builtin_ctzll(idle_cores);

            if (KThread *suggested = priority_queue.GetSuggestedFront(core_id); suggested != nullptr) {
                s32 migration_candidates[cpu::NumCores];
                size_t num_candidates = 0;

                /* While we have a suggested thread, try to migrate it! */
                while (suggested != nullptr) {
                    /* Check if the suggested thread is the top thread on its core. */
                    const s32 suggested_core = suggested->GetActiveCore();
                    if (KThread *top_thread = (suggested_core >= 0) ? top_threads[suggested_core] : nullptr; top_thread != suggested) {
                        /* Make sure we're not dealing with threads too high priority for migration. */
                        if (top_thread != nullptr && top_thread->GetPriority() < HighestCoreMigrationAllowedPriority) {
                            break;
                        }

                        /* The suggested thread isn't bound to its core, so we can migrate it! */
                        suggested->SetActiveCore(core_id);
                        priority_queue.ChangeCore(suggested_core, suggested);
                        top_threads[core_id] = suggested;
                        cores_needing_scheduling |= Kernel::GetScheduler(core_id).UpdateHighestPriorityThread(top_threads[core_id]);
                        break;
                    }

                    /* Note this core as a candidate for migration. */
                    MESOSPHERE_ASSERT(num_candidates < cpu::NumCores);
                    migration_candidates[num_candidates++] = suggested_core;
                    suggested = priority_queue.GetSuggestedNext(core_id, suggested);
                }

                /* If suggested is nullptr, we failed to migrate a specific thread. So let's try all our candidate cores' top threads. */
                if (suggested == nullptr) {
                    for (size_t i = 0; i < num_candidates; i++) {
                        /* Check if there's some other thread that can run on the candidate core. */
                        const s32 candidate_core = migration_candidates[i];
                        suggested = top_threads[candidate_core];
                        if (KThread *next_on_candidate_core = priority_queue.GetScheduledNext(candidate_core, suggested); next_on_candidate_core != nullptr) {
                            /* The candidate core can run some other thread! We'll migrate its current top thread to us. */
                            top_threads[candidate_core] = next_on_candidate_core;
                            cores_needing_scheduling |= Kernel::GetScheduler(candidate_core).UpdateHighestPriorityThread(top_threads[candidate_core]);

                            /* Perform the migration. */
                            suggested->SetActiveCore(core_id);
                            priority_queue.ChangeCore(candidate_core, suggested);
                            top_threads[core_id] = suggested;
                            cores_needing_scheduling |= Kernel::GetScheduler(core_id).UpdateHighestPriorityThread(top_threads[core_id]);
                            break;
                        }
                    }
                }
            }

            idle_cores &= ~(1ul << core_id);
        }

        return cores_needing_scheduling;
    }

    void KScheduler::SetInterruptTaskThreadRunnable() {
        MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() == 1);

        KThread *task_thread = Kernel::GetInterruptTaskManager().GetThread();
        {
            KScopedSchedulerLock sl;
            if (AMS_LIKELY(task_thread->GetState() == KThread::ThreadState_Waiting)) {
                task_thread->SetState(KThread::ThreadState_Runnable);
            }
        }
    }

    void KScheduler::SwitchThread(KThread *next_thread) {
        KProcess *cur_process = GetCurrentProcessPointer();
        KThread  *cur_thread  = GetCurrentThreadPointer();

        /* We never want to schedule a null thread, so use the idle thread if we don't have a next. */
        if (next_thread == nullptr) {
            next_thread = this->idle_thread;
        }

        /* If we're not actually switching thread, there's nothing to do. */
        if (next_thread == cur_thread) {
            return;
        }

        /* Next thread is now known not to be nullptr, and must not be dispatchable. */
        MESOSPHERE_ASSERT(next_thread->GetDisableDispatchCount() == 1);

        /* Update the CPU time tracking variables. */
        const s64 prev_tick = this->last_context_switch_time;
        const s64 cur_tick  = KHardwareTimer::GetTick();
        const s64 tick_diff = cur_tick - prev_tick;
        cur_thread->AddCpuTime(tick_diff);
        if (cur_process != nullptr) {
            cur_process->AddCpuTime(tick_diff);
        }
        this->last_context_switch_time = cur_tick;

        /* Update our previous thread. */
        if (cur_process != nullptr) {
            /* NOTE: Combining this into AMS_LIKELY(!... && ...) triggers an internal compiler error: Segmentation fault in GCC 9.2.0. */
            if (AMS_LIKELY(!cur_thread->IsTerminationRequested()) && AMS_LIKELY(cur_thread->GetActiveCore() == this->core_id)) {
                this->prev_thread = cur_thread;
            } else {
                this->prev_thread = nullptr;
            }
        } else if (cur_thread == this->idle_thread) {
            this->prev_thread = nullptr;
        }

        /* Switch the current process, if we're switching processes. */
        if (KProcess *next_process = next_thread->GetOwnerProcess(); next_process != cur_process) {
            KProcess::Switch(cur_process, next_process);
        }

        /* Set the new thread. */
        SetCurrentThread(next_thread);

        /* Set the new Thread Local region. */
        cpu::SwitchThreadLocalRegion(GetInteger(next_thread->GetThreadLocalRegionAddress()));
    }

    void KScheduler::OnThreadStateChanged(KThread *thread, KThread::ThreadState old_state) {
        MESOSPHERE_ASSERT(IsSchedulerLockedByCurrentThread());

        /* Check if the state has changed, because if it hasn't there's nothing to do. */
        const KThread::ThreadState cur_state = thread->GetRawState();
        if (cur_state == old_state) {
            return;
        }

        /* Update the priority queues. */
        if (old_state == KThread::ThreadState_Runnable) {
            /* If we were previously runnable, then we're not runnable now, and we should remove. */
            GetPriorityQueue().Remove(thread);
            IncrementScheduledCount(thread);
            SetSchedulerUpdateNeeded();
        } else if (cur_state == KThread::ThreadState_Runnable) {
            /* If we're now runnable, then we weren't previously, and we should add. */
            GetPriorityQueue().PushBack(thread);
            IncrementScheduledCount(thread);
            SetSchedulerUpdateNeeded();
        }
    }

    void KScheduler::OnThreadPriorityChanged(KThread *thread, s32 old_priority) {
        MESOSPHERE_ASSERT(IsSchedulerLockedByCurrentThread());

        /* If the thread is runnable, we want to change its priority in the queue. */
        if (thread->GetRawState() == KThread::ThreadState_Runnable) {
            GetPriorityQueue().ChangePriority(old_priority, thread == GetCurrentThreadPointer(), thread);
            IncrementScheduledCount(thread);
            SetSchedulerUpdateNeeded();
        }
    }

    void KScheduler::OnThreadAffinityMaskChanged(KThread *thread, const KAffinityMask &old_affinity, s32 old_core) {
        MESOSPHERE_ASSERT(IsSchedulerLockedByCurrentThread());

        /* If the thread is runnable, we want to change its affinity in the queue. */
        if (thread->GetRawState() == KThread::ThreadState_Runnable) {
            GetPriorityQueue().ChangeAffinityMask(old_core, old_affinity, thread);
            IncrementScheduledCount(thread);
            SetSchedulerUpdateNeeded();
        }
    }

    void KScheduler::RotateScheduledQueue(s32 core_id, s32 priority) {
        MESOSPHERE_ASSERT(IsSchedulerLockedByCurrentThread());

        /* Get a reference to the priority queue. */
        auto &priority_queue = GetPriorityQueue();

        /* Rotate the front of the queue to the end. */
        KThread *top_thread = priority_queue.GetScheduledFront(core_id, priority);
        KThread *next_thread = nullptr;
        if (top_thread != nullptr) {
            next_thread = priority_queue.MoveToScheduledBack(top_thread);
            if (next_thread != top_thread) {
                IncrementScheduledCount(top_thread);
                IncrementScheduledCount(next_thread);
            }
        }

        /* While we have a suggested thread, try to migrate it! */
        {
            KThread *suggested = priority_queue.GetSuggestedFront(core_id, priority);
            while (suggested != nullptr) {
                /* Check if the suggested thread is the top thread on its core. */
                const s32 suggested_core = suggested->GetActiveCore();
                if (KThread *top_on_suggested_core = (suggested_core >= 0) ? priority_queue.GetScheduledFront(suggested_core) : nullptr; top_on_suggested_core != suggested) {
                    /* If the next thread is a new thread that has been waiting longer than our suggestion, we prefer it to our suggestion. */
                    if (top_thread != next_thread && next_thread != nullptr && next_thread->GetLastScheduledTick() < suggested->GetLastScheduledTick()) {
                        suggested = nullptr;
                        break;
                    }

                    /* If we're allowed to do a migration, do one. */
                    /* NOTE: Unlike migrations in UpdateHighestPriorityThread, this moves the suggestion to the front of the queue. */
                    if (top_on_suggested_core == nullptr || top_on_suggested_core->GetPriority() >= HighestCoreMigrationAllowedPriority) {
                        suggested->SetActiveCore(core_id);
                        priority_queue.ChangeCore(suggested_core, suggested, true);
                        IncrementScheduledCount(suggested);
                        break;
                    }
                }

                /* Get the next suggestion. */
                suggested = priority_queue.GetSamePriorityNext(core_id, suggested);
            }
        }

        /* Now that we might have migrated a thread with the same priority, check if we can do better. */
        {
            KThread *best_thread = priority_queue.GetScheduledFront(core_id);
            if (best_thread == GetCurrentThreadPointer()) {
                best_thread = priority_queue.GetScheduledNext(core_id, best_thread);
            }

            /* If the best thread we can choose has a priority the same or worse than ours, try to migrate a higher priority thread. */
            if (best_thread != nullptr && best_thread->GetPriority() >= priority) {
                KThread *suggested = priority_queue.GetSuggestedFront(core_id);
                while (suggested != nullptr) {
                    /* If the suggestion's priority is the same as ours, don't bother. */
                    if (suggested->GetPriority() >= best_thread->GetPriority()) {
                        break;
                    }

                    /* Check if the suggested thread is the top thread on its core. */
                    const s32 suggested_core = suggested->GetActiveCore();
                    if (KThread *top_on_suggested_core = (suggested_core >= 0) ? priority_queue.GetScheduledFront(suggested_core) : nullptr; top_on_suggested_core != suggested) {
                        /* If we're allowed to do a migration, do one. */
                        /* NOTE: Unlike migrations in UpdateHighestPriorityThread, this moves the suggestion to the front of the queue. */
                        if (top_on_suggested_core == nullptr || top_on_suggested_core->GetPriority() >= HighestCoreMigrationAllowedPriority) {
                            suggested->SetActiveCore(core_id);
                            priority_queue.ChangeCore(suggested_core, suggested, true);
                            IncrementScheduledCount(suggested);
                            break;
                        }
                    }

                    /* Get the next suggestion. */
                    suggested = priority_queue.GetSuggestedNext(core_id, suggested);
                }
            }
        }

        /* After a rotation, we need a scheduler update. */
        SetSchedulerUpdateNeeded();
    }


}
