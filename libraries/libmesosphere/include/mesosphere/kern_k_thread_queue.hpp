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
            using Entry = KThread::QueueEntry;
        private:
            Entry root;
        public:
            constexpr ALWAYS_INLINE KThreadQueue() : root() { /* ... */ }

            constexpr ALWAYS_INLINE bool IsEmpty() const { return this->root.GetNext() == nullptr; }

            constexpr ALWAYS_INLINE KThread *GetFront() const { return this->root.GetNext(); }
            constexpr ALWAYS_INLINE KThread *GetNext(KThread *t) const { return t->GetSleepingQueueEntry().GetNext(); }
        private:
            constexpr ALWAYS_INLINE KThread *GetBack() const { return this->root.GetPrev(); }

            constexpr ALWAYS_INLINE void Enqueue(KThread *add) {
                /* Get the entry associated with the added thread. */
                Entry &add_entry = add->GetSleepingQueueEntry();

                /* Get the entry associated with the end of the queue. */
                KThread *tail       = this->GetBack();
                Entry   &tail_entry = (tail != nullptr) ? tail->GetSleepingQueueEntry() : this->root;

                /* Link the entries. */
                add_entry.SetPrev(tail);
                add_entry.SetNext(nullptr);
                tail_entry.SetNext(add);
                this->root.SetPrev(add);
            }

            constexpr ALWAYS_INLINE void Remove(KThread *remove) {
                /* Get the entry associated with the thread. */
                Entry &remove_entry = remove->GetSleepingQueueEntry();

                /* Get the entries associated with next and prev. */
                KThread *prev = remove_entry.GetPrev();
                KThread *next = remove_entry.GetNext();
                Entry   &prev_entry = (prev != nullptr) ? prev->GetSleepingQueueEntry() : this->root;
                Entry   &next_entry = (next != nullptr) ? next->GetSleepingQueueEntry() : this->root;

                /* Unlink. */
                prev_entry.SetNext(next);
                next_entry.SetPrev(prev);
            }
        public:
            constexpr ALWAYS_INLINE void Dequeue() {
                /* Get the front of the queue. */
                KThread *head = this->GetFront();
                if (head == nullptr) {
                    return;
                }

                MESOSPHERE_ASSERT(head->GetState() == KThread::ThreadState_Waiting);

                /* Get the entry for the next head. */
                KThread *next = GetNext(head);
                Entry   &next_entry = (next != nullptr) ? next->GetSleepingQueueEntry() : this->root;

                /* Link the entries. */
                this->root.SetNext(next);
                next_entry.SetPrev(nullptr);

                /* Clear the head's queue. */
                head->SetSleepingQueue(nullptr);
            }

            bool SleepThread(KThread *t) {
                /* Set the thread's queue and mark it as waiting. */
                t->SetSleepingQueue(this);
                t->SetState(KThread::ThreadState_Waiting);

                /* Add the thread to the queue. */
                this->Enqueue(t);

                /* If the thread needs terminating, undo our work. */
                if (t->IsTerminationRequested()) {
                    this->WakeupThread(t);
                    return false;
                }

                return true;
            }

            void WakeupThread(KThread *t) {
                MESOSPHERE_ASSERT(t->GetState() == KThread::ThreadState_Waiting);

                /* Remove the thread from the queue. */
                this->Remove(t);

                /* Mark the thread as no longer sleeping. */
                t->SetState(KThread::ThreadState_Runnable);
                t->SetSleepingQueue(nullptr);
            }

            KThread *WakeupFrontThread() {
                KThread *front = this->GetFront();
                if (front != nullptr) {
                    MESOSPHERE_ASSERT(front->GetState() == KThread::ThreadState_Waiting);

                    /* Remove the thread from the queue. */
                    this->Dequeue();

                    /* Mark the thread as no longer sleeping. */
                    front->SetState(KThread::ThreadState_Runnable);
                    front->SetSleepingQueue(nullptr);
                }
                return front;
            }
    };

}
