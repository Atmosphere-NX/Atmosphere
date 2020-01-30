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

    void NotifyAvailable() {
        /* TODO: Implement this. */
        MESOSPHERE_ABORT();
    }

    void NotifyAbort(Result abort_reason) {
        MESOSPHERE_ABORT();
    }

    void KSynchronizationObject::Finalize() {
        this->OnFinalizeSynchronizationObject();
        KAutoObject::Finalize();
    }

    void KSynchronizationObject::DebugWaiters() {
        /* TODO: Do useful debug operation here. */
    }

    KSynchronizationObject::iterator KSynchronizationObject::AddWaiterThread(KThread *thread) {
        return this->thread_list.insert(this->thread_list.end(), *thread);
    }

    KSynchronizationObject::iterator KSynchronizationObject::RemoveWaiterThread(KSynchronizationObject::iterator it) {
        return this->thread_list.erase(it);
    }

    KSynchronizationObject::iterator KSynchronizationObject::begin() {
        return this->thread_list.begin();
    }

    KSynchronizationObject::iterator KSynchronizationObject::end() {
        return this->thread_list.end();
    }

}
