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
#include "os_mutex.hpp"
#include "os_condvar.hpp"

namespace ams::os {

    namespace impl {

        class WaitableObjectList;
        class WaitableHolderOfSemaphore;

    }

    class Semaphore {
        friend class impl::WaitableHolderOfSemaphore;
        NON_COPYABLE(Semaphore);
        NON_MOVEABLE(Semaphore);
        private:
            util::TypedStorage<impl::WaitableObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitlist;
            os::Mutex mutex;
            os::ConditionVariable condvar;
            int count;
            int max_count;
        public:
            explicit Semaphore(int c, int mc);
            ~Semaphore();

            void Acquire();
            bool TryAcquire();
            bool TimedAcquire(u64 timeout);

            void Release();
            void Release(int count);

            constexpr inline int GetCurrentCount() const {
                return this->count;
            }
    };

}
