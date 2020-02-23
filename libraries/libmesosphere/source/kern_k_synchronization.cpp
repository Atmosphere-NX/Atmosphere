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
#include <mesosphere.hpp>

namespace ams::kern {

    Result KSynchronization::Wait(s32 *out_index, KSynchronizationObject **objects, const s32 num_objects, s64 timeout) {
        MESOSPHERE_ASSERT_THIS();

        MESOSPHERE_UNIMPLEMENTED();
    }

    void KSynchronization::OnAvailable(KSynchronizationObject *object) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* If we're not signaled, we've nothing to notify. */
        if (!object->IsSignaled()) {
            return;
        }

        /* Iterate over each thread. */
        for (auto &thread : *object) {
            if (thread.GetState() == KThread::ThreadState_Waiting) {
                thread.SetSyncedObject(object, ResultSuccess());
                thread.SetState(KThread::ThreadState_Runnable);
            }
        }
    }

    void KSynchronization::OnAbort(KSynchronizationObject *object, Result abort_reason) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Iterate over each thread. */
        for (auto &thread : *object) {
            if (thread.GetState() == KThread::ThreadState_Waiting) {
                thread.SetSyncedObject(object, abort_reason);
                thread.SetState(KThread::ThreadState_Runnable);
            }
        }
    }

}
