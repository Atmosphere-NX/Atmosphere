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

    void KSynchronizationObject::NotifyAvailable() {
        MESOSPHERE_ASSERT_THIS();

        Kernel::GetSynchronization().OnAvailable(this);
    }

    void KSynchronizationObject::NotifyAbort(Result abort_reason) {
        MESOSPHERE_ASSERT_THIS();

        Kernel::GetSynchronization().OnAbort(this, abort_reason);
    }

    void KSynchronizationObject::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* If auditing, ensure that the object has no waiters. */
        #if defined(MESOSPHERE_BUILD_FOR_AUDITING)
        {
            KScopedSchedulerLock sl;

            auto end = this->end();
            for (auto it = this->begin(); it != end; ++it) {
                KThread *thread = std::addressof(*it);
                MESOSPHERE_LOG("KSynchronizationObject::Finalize(%p) with %p (id=%ld) waiting.\n", this, thread, thread->GetId());
            }
        }
        #endif

        this->OnFinalizeSynchronizationObject();
        KAutoObject::Finalize();
    }

    void KSynchronizationObject::DebugWaiters() {
        MESOSPHERE_ASSERT_THIS();

        /* If debugging, dump the list of waiters. */
        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        {
            KScopedSchedulerLock sl;

            MESOSPHERE_RELEASE_LOG("Threads waiting on %p:\n", this);

            bool has_waiters = false;
            auto end = this->end();
            for (auto it = this->begin(); it != end; ++it) {
                KThread *thread = std::addressof(*it);

                if (KProcess *process = thread->GetOwnerProcess(); process != nullptr) {
                    MESOSPHERE_RELEASE_LOG("    %p tid=%ld pid=%ld (%s)\n", thread, thread->GetId(), process->GetId(), process->GetName());
                } else {
                    MESOSPHERE_RELEASE_LOG("    %p tid=%ld (Kernel)\n", thread, thread->GetId());
                }

                has_waiters = true;
            }

            /* If we didn't have any waiters, print so. */
            if (!has_waiters) {
                MESOSPHERE_RELEASE_LOG("    None\n");
            }
        }
        #endif
    }

    KSynchronizationObject::iterator KSynchronizationObject::RegisterWaitingThread(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();

        return this->thread_list.insert(this->thread_list.end(), *thread);
    }

    KSynchronizationObject::iterator KSynchronizationObject::UnregisterWaitingThread(KSynchronizationObject::iterator it) {
        MESOSPHERE_ASSERT_THIS();

        return this->thread_list.erase(it);
    }

    KSynchronizationObject::iterator KSynchronizationObject::begin() {
        MESOSPHERE_ASSERT_THIS();

        return this->thread_list.begin();
    }

    KSynchronizationObject::iterator KSynchronizationObject::end() {
        MESOSPHERE_ASSERT_THIS();

        return this->thread_list.end();
    }

}
