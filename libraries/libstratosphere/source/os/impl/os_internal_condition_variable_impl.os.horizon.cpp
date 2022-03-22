/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "os_timeout_helper.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    void InternalConditionVariableImpl::Signal() {
        ams::svc::SignalProcessWideKey(reinterpret_cast<uintptr_t>(std::addressof(m_value)), 1);
    }

    void InternalConditionVariableImpl::Broadcast() {
        ams::svc::SignalProcessWideKey(reinterpret_cast<uintptr_t>(std::addressof(m_value)), -1);
    }

    void InternalConditionVariableImpl::Wait(InternalCriticalSection *cs) {
        const auto cur_handle = GetCurrentThreadHandle();
        AMS_ASSERT((cs->Get()->m_thread_handle & ~ams::svc::HandleWaitMask) == cur_handle);

        R_ABORT_UNLESS(ams::svc::WaitProcessWideKeyAtomic(reinterpret_cast<uintptr_t>(std::addressof(cs->Get()->m_thread_handle)), reinterpret_cast<uintptr_t>(std::addressof(m_value)), cur_handle, -1));
    }

    ConditionVariableStatus InternalConditionVariableImpl::TimedWait(InternalCriticalSection *cs, const TimeoutHelper &timeout_helper) {
        const auto cur_handle = GetCurrentThreadHandle();
        AMS_ASSERT((cs->Get()->m_thread_handle & ~ams::svc::HandleWaitMask) == cur_handle);

        const TimeSpan left = timeout_helper.GetTimeLeftOnTarget();
        if (left > 0) {
            R_TRY_CATCH(ams::svc::WaitProcessWideKeyAtomic(reinterpret_cast<uintptr_t>(std::addressof(cs->Get()->m_thread_handle)), reinterpret_cast<uintptr_t>(std::addressof(m_value)), cur_handle, left.GetNanoSeconds())) {
                R_CATCH(svc::ResultTimedOut) {
                    cs->Enter();
                    return ConditionVariableStatus::TimedOut;
                }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            return ConditionVariableStatus::Success;
        } else {
            return ConditionVariableStatus::TimedOut;
        }
    }

}
