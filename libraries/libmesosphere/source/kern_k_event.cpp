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

    void KEvent::Initialize() {
        MESOSPHERE_ASSERT_THIS();

        /* Increment reference count. */
        /* Because reference count is one on creation, this will result */
        /* in a reference count of two. Thus, when both readable and */
        /* writable events are closed this object will be destroyed. */
        this->Open();

        /* Create our sub events. */
        KAutoObject::Create(std::addressof(this->readable_event));
        KAutoObject::Create(std::addressof(this->writable_event));

        /* Initialize our sub sessions. */
        this->readable_event.Initialize(this);
        this->writable_event.Initialize(this);

        /* Set our owner process. */
        this->owner = GetCurrentProcessPointer();
        this->owner->Open();

        /* Mark initialized. */
        this->initialized = true;
    }

    void KEvent::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList>::Finalize();
    }

    void KEvent::PostDestroy(uintptr_t arg) {
        /* Release the event count resource the owner process holds. */
        KProcess *owner = reinterpret_cast<KProcess *>(arg);
        owner->ReleaseResource(ams::svc::LimitableResource_EventCountMax, 1);
        owner->Close();
    }

}
