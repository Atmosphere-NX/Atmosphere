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

#pragma once
#include "os_mutex.hpp"
#include "os_condvar.hpp"
#include "os_timeout_helper.hpp"

namespace sts::os {

    class Event {
        NON_COPYABLE(Event);
        NON_MOVEABLE(Event);
        private:
            Mutex m;
            ConditionVariable cv;
            bool auto_clear;
            bool signaled;
            u64 counter = 0;
        public:
            Event(bool a = true, bool s = false) : auto_clear(a), signaled(s) {}

            void Signal() {
                std::scoped_lock lk(this->m);

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

                /* TODO: Waitable auto-wakeup. */
            }

            void Reset() {
                std::scoped_lock lk(this->m);
                this->signaled = false;
            }

            void Wait() {
                std::scoped_lock lk(this->m);

                u64 cur_counter = this->counter;
                while (!this->signaled) {
                    if (this->counter != cur_counter) {
                        break;
                    }
                    this->cv.Wait(&this->m);
                }

                if (this->auto_clear) {
                    this->signaled = false;
                }
            }

            bool TryWait() {
                std::scoped_lock lk(this->m);

                const bool success = this->signaled;
                if (this->auto_clear) {
                    this->signaled = false;
                }

                return success;
            }

            bool TimedWait(u64 ns) {
                TimeoutHelper timeout_helper(ns);
                std::scoped_lock lk(this->m);

                u64 cur_counter = this->counter;
                while (!this->signaled) {
                    if (this->counter != cur_counter) {
                        break;
                    }
                    if (R_FAILED(this->cv.TimedWait(&this->m, timeout_helper.NsUntilTimeout()))) {
                        return false;
                    }
                }

                if (this->auto_clear) {
                    this->signaled = false;
                }

                return true;
            }
    };

}
