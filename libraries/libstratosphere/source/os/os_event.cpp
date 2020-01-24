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
#include "impl/os_waitable_object_list.hpp"

namespace ams::os {

    Event::Event(bool a, bool s) : auto_clear(a), signaled(s) {
        new (GetPointer(this->waitable_object_list_storage)) impl::WaitableObjectList();
    }

    Event::~Event() {
        GetReference(this->waitable_object_list_storage).~WaitableObjectList();
    }

    void Event::Signal() {
        std::scoped_lock lk(this->lock);

        /* If we're already signaled, nothing more to do. */
        if (this->signaled) {
            return;
        }

        this->signaled = true;

        /* Signal! */
        if (this->auto_clear) {
            /* If we're auto clear, signal one thread, which will clear. */
            this->cv.Signal();
        } else {
            /* If we're manual clear, increment counter and wake all. */
            this->counter++;
            this->cv.Broadcast();
        }

        /* Wake up whatever manager, if any. */
        GetReference(this->waitable_object_list_storage).SignalAllThreads();
    }

    void Event::Reset() {
        std::scoped_lock lk(this->lock);
        this->signaled = false;
    }

    void Event::Wait() {
        std::scoped_lock lk(this->lock);

        u64 cur_counter = this->counter;
        while (!this->signaled) {
            if (this->counter != cur_counter) {
                break;
            }
            this->cv.Wait(&this->lock);
        }

        if (this->auto_clear) {
            this->signaled = false;
        }
    }

    bool Event::TryWait() {
        std::scoped_lock lk(this->lock);

        const bool success = this->signaled;
        if (this->auto_clear) {
            this->signaled = false;
        }

        return success;
    }

    bool Event::TimedWait(u64 ns) {
        TimeoutHelper timeout_helper(ns);
        std::scoped_lock lk(this->lock);

        u64 cur_counter = this->counter;
        while (!this->signaled) {
            if (this->counter != cur_counter) {
                break;
            }
            if (this->cv.TimedWait(&this->lock, timeout_helper.NsUntilTimeout()) == ConditionVariableStatus::TimedOut) {
                return false;
            }
        }

        if (this->auto_clear) {
            this->signaled = false;
        }

        return true;
    }

}
