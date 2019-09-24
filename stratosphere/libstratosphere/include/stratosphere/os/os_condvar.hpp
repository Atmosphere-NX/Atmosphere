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

namespace sts::os {

    class ConditionVariable {
        NON_COPYABLE(ConditionVariable);
        NON_MOVEABLE(ConditionVariable);
        private:
            CondVar cv;
        public:
            ConditionVariable() {
                condvarInit(&cv);
            }

            Result TimedWait(::Mutex *m, u64 timeout) {
                return condvarWaitTimeout(&cv, m, timeout);
            }

            Result Wait(::Mutex *m) {
                return condvarWait(&cv, m);
            }

            Result TimedWait(os::Mutex *m, u64 timeout) {
                return TimedWait(m->GetMutex(), timeout);
            }

            Result Wait(os::Mutex *m) {
                return Wait(m->GetMutex());
            }

            Result Wake(int num) {
                return condvarWake(&cv, num);
            }

            Result WakeOne() {
                return condvarWakeOne(&cv);
            }

            Result WakeAll() {
                return condvarWakeAll(&cv);
            }

            Result Signal() {
                return this->WakeOne();
            }

            Result Broadcast() {
                return this->WakeAll();
            }
    };

}