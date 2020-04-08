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
#include <stratosphere.hpp>
#include "impl/os_thread_manager.hpp"
#include "impl/os_timeout_helper.hpp"
#include "impl/os_mutex_impl.hpp"

namespace ams::os {

    void InitializeConditionVariable(ConditionVariableType *cv) {
        /* Construct object. */
        new (GetPointer(cv->_storage)) impl::InternalConditionVariable;

        /* Mark initialized. */
        cv->state = ConditionVariableType::State_Initialized;
    }

    void FinalizeConditionVariable(ConditionVariableType *cv) {
        AMS_ASSERT(cv->state == ConditionVariableType::State_Initialized);

        /* Mark not initialized. */
        cv->state = ConditionVariableType::State_NotInitialized;

        /* Destroy objects. */
        GetReference(cv->_storage).~InternalConditionVariable();
    }

    void SignalConditionVariable(ConditionVariableType *cv) {
        AMS_ASSERT(cv->state == ConditionVariableType::State_Initialized);

        GetReference(cv->_storage).Signal();
    }

    void BroadcastConditionVariable(ConditionVariableType *cv) {
        AMS_ASSERT(cv->state == ConditionVariableType::State_Initialized);

        GetReference(cv->_storage).Broadcast();
    }

    void WaitConditionVariable(ConditionVariableType *cv, MutexType *m) {
        AMS_ASSERT(cv->state == ConditionVariableType::State_Initialized);

        AMS_ASSERT(m->state        == MutexType::State_Initialized);
        AMS_ASSERT(m->owner_thread == impl::GetCurrentThread());
        AMS_ASSERT(m->nest_count   == 1);

        impl::PopAndCheckLockLevel(m);

        if ((--m->nest_count) == 0) {
            m->owner_thread = nullptr;
        }

        GetReference(cv->_storage).Wait(GetPointer(m->_storage));

        impl::PushAndCheckLockLevel(m);

        ++m->nest_count;
        m->owner_thread = impl::GetCurrentThread();
    }

    ConditionVariableStatus TimedWaitConditionVariable(ConditionVariableType *cv, MutexType *m, TimeSpan timeout) {
        AMS_ASSERT(cv->state == ConditionVariableType::State_Initialized);

        AMS_ASSERT(m->state        == MutexType::State_Initialized);
        AMS_ASSERT(m->owner_thread == impl::GetCurrentThread());
        AMS_ASSERT(m->nest_count   == 1);

        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        impl::PopAndCheckLockLevel(m);

        if ((--m->nest_count) == 0) {
            m->owner_thread = nullptr;
        }

        ConditionVariableStatus status;
        if (timeout == TimeSpan(0)) {
            GetReference(m->_storage).Leave();
            GetReference(m->_storage).Enter();
            status = ConditionVariableStatus::TimedOut;
        } else {
            impl::TimeoutHelper timeout_helper(timeout);
            status = GetReference(cv->_storage).TimedWait(GetPointer(m->_storage), timeout_helper);
        }

        impl::PushAndCheckLockLevel(m);

        ++m->nest_count;
        m->owner_thread = impl::GetCurrentThread();

        return status;
    }

}
