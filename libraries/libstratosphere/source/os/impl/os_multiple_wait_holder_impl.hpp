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
#pragma once
#include "os_multiple_wait_holder_of_handle.hpp"
#include "os_multiple_wait_holder_of_event.hpp"
#include "os_multiple_wait_holder_of_inter_process_event.hpp"
#include "os_multiple_wait_holder_of_interrupt_event.hpp"
#include "os_multiple_wait_holder_of_timer_event.hpp"
#include "os_multiple_wait_holder_of_thread.hpp"
#include "os_multiple_wait_holder_of_semaphore.hpp"
#include "os_multiple_wait_holder_of_message_queue.hpp"

namespace ams::os::impl {

    struct MultiWaitHolderImpl {
        union {
            util::TypedStorage<MultiWaitHolderOfHandle>                   holder_of_handle_storage;
            util::TypedStorage<MultiWaitHolderOfEvent>                    holder_of_event_storage;
            util::TypedStorage<MultiWaitHolderOfInterProcessEvent>        holder_of_inter_process_event_storage;
            util::TypedStorage<MultiWaitHolderOfInterruptEvent>           holder_of_interrupt_event_storage;
            util::TypedStorage<MultiWaitHolderOfTimerEvent>               holder_of_timer_event_storage;
            util::TypedStorage<MultiWaitHolderOfThread>                   holder_of_thread_storage;
            util::TypedStorage<MultiWaitHolderOfSemaphore>                holder_of_semaphore_storage;
            util::TypedStorage<MultiWaitHolderOfMessageQueueForNotFull>   holder_of_mq_for_not_full_storage;
            util::TypedStorage<MultiWaitHolderOfMessageQueueForNotEmpty>  holder_of_mq_for_not_empty_storage;
        };
    };

    #define CHECK_HOLDER(T) \
    static_assert(std::is_base_of<::ams::os::impl::MultiWaitHolderBase, T>::value && std::is_trivially_destructible<T>::value, #T)

    CHECK_HOLDER(MultiWaitHolderOfHandle);
    CHECK_HOLDER(MultiWaitHolderOfEvent);
    CHECK_HOLDER(MultiWaitHolderOfInterProcessEvent);
    CHECK_HOLDER(MultiWaitHolderOfInterruptEvent);
    CHECK_HOLDER(MultiWaitHolderOfTimerEvent);
    CHECK_HOLDER(MultiWaitHolderOfThread);
    CHECK_HOLDER(MultiWaitHolderOfSemaphore);
    CHECK_HOLDER(MultiWaitHolderOfMessageQueueForNotFull);
    CHECK_HOLDER(MultiWaitHolderOfMessageQueueForNotEmpty);

    #undef CHECK_HOLDER

    static_assert(std::is_trivial<MultiWaitHolderImpl>::value && std::is_trivially_destructible<MultiWaitHolderImpl>::value);
    static_assert(sizeof(MultiWaitHolderImpl) == sizeof(os::MultiWaitHolderType::impl_storage));
}
