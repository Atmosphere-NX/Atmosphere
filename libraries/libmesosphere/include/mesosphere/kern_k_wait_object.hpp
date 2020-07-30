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
#include <mesosphere/kern_k_timer_task.hpp>
#include <mesosphere/kern_k_thread.hpp>

namespace ams::kern {

    class KWaitObject : public KTimerTask {
        private:
            using Entry = KThread::QueueEntry;
        private:
            Entry root;
            bool  timer_used;
        public:
            constexpr KWaitObject() : root(), timer_used() { /* ... */ }

            virtual void OnTimer() override;
            Result Synchronize(s64 timeout);
        private:
            constexpr ALWAYS_INLINE void Enqueue(KThread *add) {
                /* Get the entry associated with the added thread. */
                Entry &add_entry = add->GetSleepingQueueEntry();

                /* Get the entry associated with the end of the queue. */
                KThread *tail       = this->root.GetPrev();
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
    };

}
