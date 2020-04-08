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
#pragma once
#include "os_waitable_holder_of_handle.hpp"
#include "os_waitable_holder_of_event.hpp"
#include "os_waitable_holder_of_inter_process_event.hpp"
#include "os_waitable_holder_of_interrupt_event.hpp"
#include "os_waitable_holder_of_thread.hpp"
#include "os_waitable_holder_of_semaphore.hpp"
#include "os_waitable_holder_of_message_queue.hpp"

namespace ams::os::impl {

    struct WaitableHolderImpl {
        union {
            TYPED_STORAGE(WaitableHolderOfHandle)                   holder_of_handle_storage;
            TYPED_STORAGE(WaitableHolderOfEvent)                    holder_of_event_storage;
            TYPED_STORAGE(WaitableHolderOfInterProcessEvent)        holder_of_inter_process_event_storage;
            TYPED_STORAGE(WaitableHolderOfInterruptEvent)           holder_of_interrupt_event_storage;
            TYPED_STORAGE(WaitableHolderOfThread)                   holder_of_thread_storage;
            TYPED_STORAGE(WaitableHolderOfSemaphore)                holder_of_semaphore_storage;
            TYPED_STORAGE(WaitableHolderOfMessageQueueForNotFull)   holder_of_mq_for_not_full_storage;
            TYPED_STORAGE(WaitableHolderOfMessageQueueForNotEmpty)  holder_of_mq_for_not_empty_storage;
        };
    };

    #define CHECK_HOLDER(T) \
    static_assert(std::is_base_of<::ams::os::impl::WaitableHolderBase, T>::value && std::is_trivially_destructible<T>::value, #T)

    CHECK_HOLDER(WaitableHolderOfHandle);
    CHECK_HOLDER(WaitableHolderOfEvent);
    CHECK_HOLDER(WaitableHolderOfInterProcessEvent);
    CHECK_HOLDER(WaitableHolderOfInterruptEvent);
    CHECK_HOLDER(WaitableHolderOfThread);
    CHECK_HOLDER(WaitableHolderOfSemaphore);
    CHECK_HOLDER(WaitableHolderOfMessageQueueForNotFull);
    CHECK_HOLDER(WaitableHolderOfMessageQueueForNotEmpty);

    #undef CHECK_HOLDER

    static_assert(std::is_trivial<WaitableHolderImpl>::value && std::is_trivially_destructible<WaitableHolderImpl>::value);
    static_assert(sizeof(WaitableHolderImpl) == sizeof(os::WaitableHolderType::impl_storage));
}
