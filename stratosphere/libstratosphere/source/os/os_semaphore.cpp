/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

    Semaphore::Semaphore(int c, int mc) : count(c), max_count(mc) {
        new (GetPointer(this->waitlist)) impl::WaitableObjectList();
    }

    Semaphore::~Semaphore() {
        GetReference(this->waitlist).~WaitableObjectList();
    }

    void Semaphore::Acquire() {
        std::scoped_lock lk(this->mutex);

        while (this->count == 0) {
            this->condvar.Wait(&this->mutex);
        }

        this->count--;
    }

    bool Semaphore::TryAcquire() {
        std::scoped_lock lk(this->mutex);

        if (this->count == 0) {
            return false;
        }

        this->count--;
        return true;
    }

    bool Semaphore::TimedAcquire(u64 timeout) {
        std::scoped_lock lk(this->mutex);
        TimeoutHelper timeout_helper(timeout);

        while (this->count == 0) {
            if (timeout_helper.TimedOut()) {
                return false;
            }

            this->condvar.TimedWait(&this->mutex, timeout_helper.NsUntilTimeout());
        }

        this->count--;
        return true;
    }

    void Semaphore::Release() {
        std::scoped_lock lk(this->mutex);

        AMS_ASSERT(this->count + 1 <= this->max_count);
        this->count++;

        this->condvar.Signal();
        GetReference(this->waitlist).SignalAllThreads();
    }

    void Semaphore::Release(int count) {
        std::scoped_lock lk(this->mutex);

        AMS_ASSERT(this->count + count <= this->max_count);
        this->count += count;

        this->condvar.Broadcast();
        GetReference(this->waitlist).SignalAllThreads();
    }

}
