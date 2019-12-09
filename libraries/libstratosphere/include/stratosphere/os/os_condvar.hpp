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

namespace ams::os {

    enum class ConditionVariableStatus {
        TimedOut = 0,
        Success  = 1,
    };

    class ConditionVariable {
        NON_COPYABLE(ConditionVariable);
        NON_MOVEABLE(ConditionVariable);
        private:
            CondVar cv;
        public:
            ConditionVariable() {
                condvarInit(&cv);
            }

            ConditionVariableStatus TimedWait(::Mutex *m, u64 timeout) {
                if (timeout > 0) {
                    /* Abort on any error other than timed out/success. */
                    R_TRY_CATCH(condvarWaitTimeout(&this->cv, m, timeout)) {
                        R_CATCH(svc::ResultTimedOut) { return ConditionVariableStatus::TimedOut; }
                    } R_END_TRY_CATCH_WITH_ASSERT;

                    return ConditionVariableStatus::Success;
                }
                return ConditionVariableStatus::TimedOut;
            }

            void Wait(::Mutex *m) {
                R_ASSERT(condvarWait(&this->cv, m));
            }

            ConditionVariableStatus TimedWait(os::Mutex *m, u64 timeout) {
                return this->TimedWait(m->GetMutex(), timeout);
            }

            void Wait(os::Mutex *m) {
                return this->Wait(m->GetMutex());
            }

            void Signal() {
                condvarWakeOne(&this->cv);
            }

            void Broadcast() {
                condvarWakeAll(&this->cv);
            }
    };

}