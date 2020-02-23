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

        this->OnFinalizeSynchronizationObject();
        KAutoObject::Finalize();
    }

    void KSynchronizationObject::DebugWaiters() {
        MESOSPHERE_ASSERT_THIS();

        MESOSPHERE_TODO("Do useful debug operation here.");
    }

    KSynchronizationObject::iterator KSynchronizationObject::AddWaiterThread(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();

        return this->thread_list.insert(this->thread_list.end(), *thread);
    }

    KSynchronizationObject::iterator KSynchronizationObject::RemoveWaiterThread(KSynchronizationObject::iterator it) {
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
