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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_priority_queue.hpp>
#include <mesosphere/kern_k_scheduler_lock.hpp>

namespace ams::kern {

    using KSchedulerPriorityQueue = KPriorityQueue<KThread, cpu::NumCores, ams::svc::LowestThreadPriority, ams::svc::HighestThreadPriority>;
    static_assert(std::is_same<KSchedulerPriorityQueue::AffinityMaskType, KAffinityMask>::value);
    static_assert(KSchedulerPriorityQueue::NumCores    == cpu::NumCores);
    static_assert(KSchedulerPriorityQueue::NumPriority == BITSIZEOF(u64));

    class KScopedSchedulerLock;
    class KScopedSchedulerLockAndSleep;

    class KScheduler {
        NON_COPYABLE(KScheduler);
        NON_MOVEABLE(KScheduler);
        public:
            static constexpr s32 HighestCoreMigrationAllowedPriority = 2;
            static_assert(ams::svc::LowestThreadPriority  >= HighestCoreMigrationAllowedPriority);
            static_assert(ams::svc::HighestThreadPriority <= HighestCoreMigrationAllowedPriority);

            struct SchedulingState {
                std::atomic<bool> needs_scheduling;
                bool interrupt_task_thread_runnable;
                bool should_count_idle;
                u64  idle_count;
                KThread *highest_priority_thread;
                void *idle_thread_stack;
            };
        private:
            friend class KScopedSchedulerLock;
            friend class KScopedSchedulerLockAndSleep;
            friend class KScopedDisableDispatch;
        private:
            SchedulingState state;
            bool is_active;
            s32 core_id;
            KThread *prev_thread;
            s64 last_context_switch_time;
            KThread *idle_thread;
            std::atomic<KThread *> current_thread;
        public:
            constexpr KScheduler()
                : state(), is_active(false), core_id(0), prev_thread(nullptr), last_context_switch_time(0), idle_thread(nullptr), current_thread(nullptr)
            {
                this->state.needs_scheduling = true;
                this->state.interrupt_task_thread_runnable = false;
                this->state.should_count_idle = false;
                this->state.idle_count = 0;
                this->state.idle_thread_stack = nullptr;
                this->state.highest_priority_thread = nullptr;
            }

            NOINLINE void Initialize(KThread *idle_thread);
            NOINLINE void Activate();

            ALWAYS_INLINE void SetInterruptTaskRunnable() {
                this->state.interrupt_task_thread_runnable = true;
                this->state.needs_scheduling               = true;
            }

            ALWAYS_INLINE void RequestScheduleOnInterrupt() {
                this->state.needs_scheduling = true;

                if (CanSchedule()) {
                    this->ScheduleOnInterrupt();
                }
            }

            ALWAYS_INLINE u64 GetIdleCount() const {
                return this->state.idle_count;
            }

            ALWAYS_INLINE KThread *GetIdleThread() const {
                return this->idle_thread;
            }

            ALWAYS_INLINE KThread *GetPreviousThread() const {
                return this->prev_thread;
            }

            ALWAYS_INLINE KThread *GetSchedulerCurrentThread() const {
                return this->current_thread;
            }

            ALWAYS_INLINE s64 GetLastContextSwitchTime() const {
                return this->last_context_switch_time;
            }
        private:
            /* Static private API. */
            static ALWAYS_INLINE KSchedulerPriorityQueue &GetPriorityQueue() { return s_priority_queue; }
            static NOINLINE u64 UpdateHighestPriorityThreadsImpl();

            static NOINLINE void InterruptTaskThreadToRunnable();
        public:
            /* Static public API. */
            static ALWAYS_INLINE bool CanSchedule() { return GetCurrentThread().GetDisableDispatchCount() == 0; }
            static ALWAYS_INLINE bool IsSchedulerLockedByCurrentThread() { return s_scheduler_lock.IsLockedByCurrentThread(); }

            static ALWAYS_INLINE bool IsSchedulerUpdateNeeded() { return s_scheduler_update_needed; }
            static ALWAYS_INLINE void SetSchedulerUpdateNeeded() { s_scheduler_update_needed = true; }
            static ALWAYS_INLINE void ClearSchedulerUpdateNeeded() { s_scheduler_update_needed = false; }

            static ALWAYS_INLINE void DisableScheduling() {
                MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() >= 0);
                GetCurrentThread().DisableDispatch();
            }

            static NOINLINE void EnableScheduling(u64 cores_needing_scheduling) {
                MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() >= 1);

                if (GetCurrentThread().GetDisableDispatchCount() > 1) {
                    GetCurrentThread().EnableDispatch();
                } else {
                    GetCurrentScheduler().RescheduleOtherCores(cores_needing_scheduling);
                    GetCurrentScheduler().RescheduleCurrentCore();
                }
            }

            static ALWAYS_INLINE u64 UpdateHighestPriorityThreads() {
                if (IsSchedulerUpdateNeeded()) {
                    return UpdateHighestPriorityThreadsImpl();
                } else {
                    return 0;
                }
            }

            static NOINLINE void ClearPreviousThread(KThread *thread);

            static NOINLINE void OnThreadStateChanged(KThread *thread, KThread::ThreadState old_state);
            static NOINLINE void OnThreadPriorityChanged(KThread *thread, s32 old_priority);
            static NOINLINE void OnThreadAffinityMaskChanged(KThread *thread, const KAffinityMask &old_affinity, s32 old_core);

            static NOINLINE void RotateScheduledQueue(s32 priority, s32 core_id);

            static NOINLINE void YieldWithoutCoreMigration();
            static NOINLINE void YieldWithCoreMigration();
            static NOINLINE void YieldToAnyThread();
        private:
            /* Instanced private API. */
            void ScheduleImpl();
            void SwitchThread(KThread *next_thread);

            ALWAYS_INLINE void Schedule() {
                MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() == 1);
                MESOSPHERE_ASSERT(this->core_id == GetCurrentCoreId());

                this->ScheduleImpl();
            }

            ALWAYS_INLINE void ScheduleOnInterrupt() {
                GetCurrentThread().DisableDispatch();
                this->Schedule();
                GetCurrentThread().EnableDispatch();
            }

            void RescheduleOtherCores(u64 cores_needing_scheduling);

            ALWAYS_INLINE void RescheduleCurrentCore() {
                MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() == 1);
                {
                    /* Disable interrupts, and then context switch. */
                    KScopedInterruptDisable intr_disable;
                    ON_SCOPE_EXIT { GetCurrentThread().EnableDispatch(); };

                    if (this->state.needs_scheduling) {
                        Schedule();
                    }
                }
            }

            NOINLINE u64 UpdateHighestPriorityThread(KThread *thread);
        public:
            using LockType = KAbstractSchedulerLock<KScheduler>;
        private:
            static bool s_scheduler_update_needed;
            static KSchedulerPriorityQueue s_priority_queue;
            static LockType s_scheduler_lock;
    };

    class KScopedSchedulerLock : KScopedLock<KScheduler::LockType> {
        public:
            explicit ALWAYS_INLINE KScopedSchedulerLock() : KScopedLock(KScheduler::s_scheduler_lock) { /* ... */ }
            ALWAYS_INLINE ~KScopedSchedulerLock() { /* ... */ }
    };

}
