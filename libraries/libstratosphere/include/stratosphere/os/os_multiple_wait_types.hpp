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
#include <vapours.hpp>
#include <stratosphere/os/impl/os_internal_critical_section.hpp>

namespace ams::os {

    namespace impl {

        class MultiWaitImpl;
        struct MultiWaitHolderImpl;

    }

    struct MultiWaitType {
        enum State {
            State_NotInitialized,
            State_Initialized,
        };

        u8 state;
        bool is_waiting;
        util::TypedStorage<impl::MultiWaitImpl, sizeof(util::IntrusiveListNode) + sizeof(impl::InternalCriticalSection) + 2 * sizeof(void *) + sizeof(Handle), alignof(void *)> impl_storage;
    };
    static_assert(std::is_trivial<MultiWaitType>::value);

    struct MultiWaitHolderType {
        util::TypedStorage<impl::MultiWaitHolderImpl, 2 * sizeof(util::IntrusiveListNode) + 3 * sizeof(void *), alignof(void *)> impl_storage;
        uintptr_t user_data;
    };
    static_assert(std::is_trivial<MultiWaitHolderType>::value);

}
