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
#include "impl/os_waitable_holder_impl.hpp"
#include "impl/os_waitable_manager_impl.hpp"

namespace ams::os {

    WaitableManager::WaitableManager() {
        /* Initialize storage. */
        new (GetPointer(this->impl_storage)) impl::WaitableManagerImpl();
    }

    WaitableManager::~WaitableManager() {
        auto &impl = GetReference(this->impl_storage);

        /* Don't allow destruction of a non-empty waitable holder. */
        AMS_ABORT_UNLESS(impl.IsEmpty());

        impl.~WaitableManagerImpl();
    }


    /* Wait. */
    WaitableHolder *WaitableManager::WaitAny() {
        auto &impl = GetReference(this->impl_storage);

        /* Don't allow waiting on empty list. */
        AMS_ABORT_UNLESS(!impl.IsEmpty());

        return reinterpret_cast<WaitableHolder *>(impl.WaitAny());
    }

    WaitableHolder *WaitableManager::TryWaitAny() {
        auto &impl = GetReference(this->impl_storage);

        /* Don't allow waiting on empty list. */
        AMS_ABORT_UNLESS(!impl.IsEmpty());

        return reinterpret_cast<WaitableHolder *>(impl.TryWaitAny());
    }

    WaitableHolder *WaitableManager::TimedWaitAny(u64 timeout) {
        auto &impl = GetReference(this->impl_storage);

        /* Don't allow waiting on empty list. */
        AMS_ABORT_UNLESS(!impl.IsEmpty());

        return reinterpret_cast<WaitableHolder *>(impl.TimedWaitAny(timeout));
    }

    /* Link. */
    void WaitableManager::LinkWaitableHolder(WaitableHolder *holder) {
        auto &impl = GetReference(this->impl_storage);
        auto holder_base = reinterpret_cast<impl::WaitableHolderBase *>(GetPointer(holder->impl_storage));

        /* Don't allow double-linking a holder. */
        AMS_ABORT_UNLESS(!holder_base->IsLinkedToManager());

        impl.LinkWaitableHolder(*holder_base);
        holder_base->SetManager(&impl);
    }

    void WaitableManager::UnlinkAll() {
        auto &impl = GetReference(this->impl_storage);
        impl.UnlinkAll();
    }

    void WaitableManager::MoveAllFrom(WaitableManager *other) {
        auto &dst_impl = GetReference(this->impl_storage);
        auto &src_impl = GetReference(other->impl_storage);

        dst_impl.MoveAllFrom(src_impl);
    }

}
