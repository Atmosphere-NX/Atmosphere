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
#include <stratosphere.hpp>
#include "impl/os_waitable_manager_impl.hpp"
#include "impl/os_waitable_holder_base.hpp"
#include "impl/os_waitable_holder_impl.hpp"

namespace ams::os {

    namespace {

        ALWAYS_INLINE impl::WaitableManagerImpl &GetWaitableManagerImpl(WaitableManagerType *manager) {
            return GetReference(manager->impl_storage);
        }

        ALWAYS_INLINE WaitableHolderType *CastToWaitableHolder(impl::WaitableHolderBase *base) {
            return reinterpret_cast<WaitableHolderType *>(base);
        }

    }

    void InitializeWaitableManager(WaitableManagerType *manager) {
        /* Initialize storage. */
        util::ConstructAt(manager->impl_storage);

        /* Mark initialized. */
        manager->state = WaitableManagerType::State_Initialized;
    }

    void FinalizeWaitableManager(WaitableManagerType *manager) {
        auto &impl = GetWaitableManagerImpl(manager);

        AMS_ASSERT(manager->state == WaitableManagerType::State_Initialized);
        AMS_ASSERT(impl.IsEmpty());

        /* Mark not initialized. */
        manager->state = WaitableManagerType::State_NotInitialized;

        /* Destroy. */
        util::DestroyAt(manager->impl_storage);
    }

    WaitableHolderType *WaitAny(WaitableManagerType *manager) {
        auto &impl = GetWaitableManagerImpl(manager);

        AMS_ASSERT(manager->state == WaitableManagerType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());

        auto *holder = CastToWaitableHolder(impl.WaitAny());
        AMS_ASSERT(holder != nullptr);
        return holder;
    }

    WaitableHolderType *TryWaitAny(WaitableManagerType *manager) {
        auto &impl = GetWaitableManagerImpl(manager);

        AMS_ASSERT(manager->state == WaitableManagerType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());

        auto *holder = CastToWaitableHolder(impl.TryWaitAny());
        return holder;
    }

    WaitableHolderType *TimedWaitAny(WaitableManagerType *manager, TimeSpan timeout) {
        auto &impl = GetWaitableManagerImpl(manager);

        AMS_ASSERT(manager->state == WaitableManagerType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        auto *holder = CastToWaitableHolder(impl.TimedWaitAny(timeout));
        return holder;
    }

    void FinalizeWaitableHolder(WaitableHolderType *holder) {
        auto *holder_base = reinterpret_cast<impl::WaitableHolderBase *>(GetPointer(holder->impl_storage));

        AMS_ASSERT(!holder_base->IsLinkedToManager());

        std::destroy_at(holder_base);
    }

    void LinkWaitableHolder(WaitableManagerType *manager, WaitableHolderType *holder) {
        auto &impl = GetWaitableManagerImpl(manager);
        auto *holder_base = reinterpret_cast<impl::WaitableHolderBase *>(GetPointer(holder->impl_storage));

        AMS_ASSERT(manager->state == WaitableManagerType::State_Initialized);
        AMS_ASSERT(!holder_base->IsLinkedToManager());

        impl.LinkWaitableHolder(*holder_base);
        holder_base->SetManager(&impl);
    }

    void UnlinkWaitableHolder(WaitableHolderType *holder) {
        auto *holder_base = reinterpret_cast<impl::WaitableHolderBase *>(GetPointer(holder->impl_storage));

        /* Don't allow unlinking of an unlinked holder. */
        AMS_ABORT_UNLESS(holder_base->IsLinkedToManager());

        holder_base->GetManager()->UnlinkWaitableHolder(*holder_base);
        holder_base->SetManager(nullptr);
    }

    void UnlinkAllWaitableHolder(WaitableManagerType *manager) {
        auto &impl = GetWaitableManagerImpl(manager);

        AMS_ASSERT(manager->state == WaitableManagerType::State_Initialized);

        return impl.UnlinkAll();
    }

    void MoveAllWaitableHolder(WaitableManagerType *_dst, WaitableManagerType *_src) {
        auto &dst = GetWaitableManagerImpl(_dst);
        auto &src = GetWaitableManagerImpl(_src);

        AMS_ASSERT(_dst->state == WaitableManagerType::State_Initialized);
        AMS_ASSERT(_src->state == WaitableManagerType::State_Initialized);

        return dst.MoveAllFrom(src);
    }

    void SetWaitableHolderUserData(WaitableHolderType *holder, uintptr_t user_data) {
        holder->user_data = user_data;
    }

    uintptr_t GetWaitableHolderUserData(const WaitableHolderType *holder) {
        return holder->user_data;
    }

    void InitializeWaitableHolder(WaitableHolderType *holder, Handle handle) {
        AMS_ASSERT(handle != svc::InvalidHandle);

        util::ConstructAt(GetReference(holder->impl_storage).holder_of_handle_storage, handle);

        holder->user_data = 0;
    }

}
