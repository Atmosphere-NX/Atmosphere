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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_thread.hpp>

namespace ams::kern {

    class KThreadQueue {
        private:
            KThread::WaiterList wait_list;
        public:
            constexpr ALWAYS_INLINE KThreadQueue() : wait_list() { /* ... */ }

            bool IsEmpty() const { return this->wait_list.empty(); }

            KThread::WaiterList::iterator begin() { return this->wait_list.begin(); }
            KThread::WaiterList::iterator end() { return this->wait_list.end(); }

            bool SleepThread(KThread *t) {
                KScopedSchedulerLock sl;

                /* If the thread needs terminating, don't enqueue it. */
                if (t->IsTerminationRequested()) {
                    return false;
                }

                /* Set the thread's queue and mark it as waiting. */
                t->SetSleepingQueue(this);
                t->SetState(KThread::ThreadState_Waiting);

                /* Add the thread to the queue. */
                this->wait_list.push_back(*t);

                return true;
            }

            void WakeupThread(KThread *t) {
                KScopedSchedulerLock sl;

                /* Remove the thread from the queue. */
                this->wait_list.erase(this->wait_list.iterator_to(*t));

                /* Mark the thread as no longer sleeping. */
                t->SetState(KThread::ThreadState_Runnable);
                t->SetSleepingQueue(nullptr);
            }

            KThread *WakeupFrontThread() {
                KScopedSchedulerLock sl;

                if (this->wait_list.empty()) {
                    return nullptr;
                } else {
                    /* Remove the thread from the queue. */
                    auto it = this->wait_list.begin();
                    KThread *thread = std::addressof(*it);
                    this->wait_list.erase(it);

                    MESOSPHERE_ASSERT(thread->GetState() == KThread::ThreadState_Waiting);

                    /* Mark the thread as no longer sleeping. */
                    thread->SetState(KThread::ThreadState_Runnable);
                    thread->SetSleepingQueue(nullptr);

                    return thread;
                }
            }
    };

}
