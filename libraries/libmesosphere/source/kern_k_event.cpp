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
#include <mesosphere.hpp>

namespace ams::kern {

    void KEvent::Initialize() {
        MESOSPHERE_ASSERT_THIS();

        /* Create our readable event. */
        KAutoObject::Create<KReadableEvent>(std::addressof(m_readable_event));

        /* Initialize our readable event. */
        m_readable_event.Initialize(this);

        /* Set our owner process. */
        m_owner = GetCurrentProcessPointer();
        m_owner->Open();

        /* Mark initialized. */
        m_initialized = true;
    }

    void KEvent::Finalize() {
        MESOSPHERE_ASSERT_THIS();
    }

    Result KEvent::Signal() {
        KScopedSchedulerLock sl;

        R_SUCCEED_IF(m_readable_event_destroyed);

        return m_readable_event.Signal();
    }

    Result KEvent::Clear() {
        KScopedSchedulerLock sl;

        R_SUCCEED_IF(m_readable_event_destroyed);

        return m_readable_event.Clear();
    }

    void KEvent::PostDestroy(uintptr_t arg) {
        /* Release the event count resource the owner process holds. */
        KProcess *owner = reinterpret_cast<KProcess *>(arg);
        owner->ReleaseResource(ams::svc::LimitableResource_EventCountMax, 1);
        owner->Close();
    }

}
