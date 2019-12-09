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
#include "impl/os_waitable_holder_impl.hpp"
#include "impl/os_waitable_manager_impl.hpp"

namespace ams::os {

    WaitableHolder::WaitableHolder(Handle handle) {
        /* Don't allow invalid handles. */
        AMS_ASSERT(handle != INVALID_HANDLE);

        /* Initialize appropriate holder. */
        new (GetPointer(this->impl_storage)) impl::WaitableHolderOfHandle(handle);

        /* Set user-data. */
        this->user_data = 0;
    }

    WaitableHolder::WaitableHolder(Event *event) {
        /* Initialize appropriate holder. */
        new (GetPointer(this->impl_storage)) impl::WaitableHolderOfEvent(event);

        /* Set user-data. */
        this->user_data = 0;
    }

    WaitableHolder::WaitableHolder(SystemEvent *event) {
        /* Initialize appropriate holder. */
        switch (event->GetState()) {
            case SystemEventState::Event:
                new (GetPointer(this->impl_storage)) impl::WaitableHolderOfEvent(&event->GetEvent());
                break;
            case SystemEventState::InterProcessEvent:
                new (GetPointer(this->impl_storage)) impl::WaitableHolderOfInterProcessEvent(&event->GetInterProcessEvent());
                break;
            case SystemEventState::Uninitialized:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Set user-data. */
        this->user_data = 0;
    }

    WaitableHolder::WaitableHolder(InterruptEvent *event) {
        /* Initialize appropriate holder. */
        new (GetPointer(this->impl_storage)) impl::WaitableHolderOfInterruptEvent(event);

        /* Set user-data. */
        this->user_data = 0;
    }

    WaitableHolder::WaitableHolder(Thread *thread) {
        /* Initialize appropriate holder. */
        new (GetPointer(this->impl_storage)) impl::WaitableHolderOfThread(thread);

        /* Set user-data. */
        this->user_data = 0;
    }

    WaitableHolder::WaitableHolder(Semaphore *semaphore) {
        /* Initialize appropriate holder. */
        new (GetPointer(this->impl_storage)) impl::WaitableHolderOfSemaphore(semaphore);

        /* Set user-data. */
        this->user_data = 0;
    }

    WaitableHolder::WaitableHolder(MessageQueue *message_queue, MessageQueueWaitKind wait_kind) {
        /* Initialize appropriate holder. */
        switch (wait_kind) {
            case MessageQueueWaitKind::ForNotFull:
                new (GetPointer(this->impl_storage)) impl::WaitableHolderOfMessageQueueForNotFull(message_queue);
                break;
            case MessageQueueWaitKind::ForNotEmpty:
                new (GetPointer(this->impl_storage)) impl::WaitableHolderOfMessageQueueForNotEmpty(message_queue);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Set user-data. */
        this->user_data = 0;
    }

    WaitableHolder::~WaitableHolder() {
        auto holder_base = reinterpret_cast<impl::WaitableHolderBase *>(GetPointer(this->impl_storage));

        /* Don't allow destruction of a linked waitable holder. */
        AMS_ASSERT(!holder_base->IsLinkedToManager());

        holder_base->~WaitableHolderBase();
    }

    void WaitableHolder::UnlinkFromWaitableManager() {
        auto holder_base = reinterpret_cast<impl::WaitableHolderBase *>(GetPointer(this->impl_storage));

        /* Don't allow unlinking of an unlinked holder. */
        AMS_ASSERT(holder_base->IsLinkedToManager());

        holder_base->GetManager()->UnlinkWaitableHolder(*holder_base);
        holder_base->SetManager(nullptr);
    }

}
