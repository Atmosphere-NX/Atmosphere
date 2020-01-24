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
#include "os_mutex.hpp"

namespace ams::os {

    class WaitableHolder;

    namespace impl {

        class WaitableManagerImpl;

    }

    class WaitableManager {
        NON_COPYABLE(WaitableManager);
        NON_MOVEABLE(WaitableManager);
        private:
            util::TypedStorage<impl::WaitableManagerImpl, sizeof(util::IntrusiveListNode) + sizeof(Mutex) + 2 * sizeof(void *) + sizeof(Handle), alignof(void *)> impl_storage;
        public:
            static constexpr size_t ImplStorageSize = sizeof(impl_storage);
        public:
            WaitableManager();
            ~WaitableManager();

            /* Wait. */
            WaitableHolder *WaitAny();
            WaitableHolder *TryWaitAny();
            WaitableHolder *TimedWaitAny(u64 timeout);

            /* Link. */
            void LinkWaitableHolder(WaitableHolder *holder);
            void UnlinkAll();
            void MoveAllFrom(WaitableManager *other);
    };

}
