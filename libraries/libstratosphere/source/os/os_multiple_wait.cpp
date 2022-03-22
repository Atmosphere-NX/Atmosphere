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
#include <stratosphere.hpp>
#include "impl/os_multiple_wait_impl.hpp"
#include "impl/os_multiple_wait_holder_base.hpp"
#include "impl/os_multiple_wait_holder_impl.hpp"

namespace ams::os {

    namespace {

        ALWAYS_INLINE impl::MultiWaitImpl &GetMultiWaitImpl(MultiWaitType *multi_wait) {
            return GetReference(multi_wait->impl_storage);
        }

        ALWAYS_INLINE MultiWaitHolderType *CastToMultiWaitHolder(impl::MultiWaitHolderBase *base) {
            return reinterpret_cast<MultiWaitHolderType *>(base);
        }

    }

    void InitializeMultiWait(MultiWaitType *multi_wait) {
        /* Initialize storage. */
        util::ConstructAt(multi_wait->impl_storage);

        /* Mark initialized. */
        multi_wait->state = MultiWaitType::State_Initialized;
    }

    void FinalizeMultiWait(MultiWaitType *multi_wait) {
        auto &impl = GetMultiWaitImpl(multi_wait);

        AMS_ASSERT(multi_wait->state == MultiWaitType::State_Initialized);
        AMS_ASSERT(impl.IsEmpty());
        AMS_UNUSED(impl);

        /* Mark not initialized. */
        multi_wait->state = MultiWaitType::State_NotInitialized;

        /* Destroy. */
        util::DestroyAt(multi_wait->impl_storage);
    }

    MultiWaitHolderType *WaitAny(MultiWaitType *multi_wait) {
        auto &impl = GetMultiWaitImpl(multi_wait);

        AMS_ASSERT(multi_wait->state == MultiWaitType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());

        auto *holder = CastToMultiWaitHolder(impl.WaitAny());
        AMS_ASSERT(holder != nullptr);
        return holder;
    }

    MultiWaitHolderType *TryWaitAny(MultiWaitType *multi_wait) {
        auto &impl = GetMultiWaitImpl(multi_wait);

        AMS_ASSERT(multi_wait->state == MultiWaitType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());

        auto *holder = CastToMultiWaitHolder(impl.TryWaitAny());
        return holder;
    }

    MultiWaitHolderType *TimedWaitAny(MultiWaitType *multi_wait, TimeSpan timeout) {
        auto &impl = GetMultiWaitImpl(multi_wait);

        AMS_ASSERT(multi_wait->state == MultiWaitType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        auto *holder = CastToMultiWaitHolder(impl.TimedWaitAny(timeout));
        return holder;
    }

    void FinalizeMultiWaitHolder(MultiWaitHolderType *holder) {
        auto *holder_base = reinterpret_cast<impl::MultiWaitHolderBase *>(GetPointer(holder->impl_storage));

        AMS_ASSERT(!holder_base->IsLinked());

        std::destroy_at(holder_base);
    }

    void LinkMultiWaitHolder(MultiWaitType *multi_wait, MultiWaitHolderType *holder) {
        auto &impl = GetMultiWaitImpl(multi_wait);
        auto *holder_base = reinterpret_cast<impl::MultiWaitHolderBase *>(GetPointer(holder->impl_storage));

        AMS_ASSERT(multi_wait->state == MultiWaitType::State_Initialized);
        AMS_ASSERT(!holder_base->IsLinked());

        impl.LinkMultiWaitHolder(*holder_base);
        holder_base->SetMultiWait(std::addressof(impl));
    }

    void UnlinkMultiWaitHolder(MultiWaitHolderType *holder) {
        auto *holder_base = reinterpret_cast<impl::MultiWaitHolderBase *>(GetPointer(holder->impl_storage));

        /* Don't allow unlinking of an unlinked holder. */
        AMS_ABORT_UNLESS(holder_base->IsLinked());

        holder_base->GetMultiWait()->UnlinkMultiWaitHolder(*holder_base);
        holder_base->SetMultiWait(nullptr);
    }

    void UnlinkAllMultiWaitHolder(MultiWaitType *multi_wait) {
        auto &impl = GetMultiWaitImpl(multi_wait);

        AMS_ASSERT(multi_wait->state == MultiWaitType::State_Initialized);

        return impl.UnlinkAll();
    }

    void MoveAllMultiWaitHolder(MultiWaitType *_dst, MultiWaitType *_src) {
        auto &dst = GetMultiWaitImpl(_dst);
        auto &src = GetMultiWaitImpl(_src);

        AMS_ASSERT(_dst->state == MultiWaitType::State_Initialized);
        AMS_ASSERT(_src->state == MultiWaitType::State_Initialized);

        return dst.MoveAllFrom(src);
    }

    void SetMultiWaitHolderUserData(MultiWaitHolderType *holder, uintptr_t user_data) {
        holder->user_data = user_data;
    }

    uintptr_t GetMultiWaitHolderUserData(const MultiWaitHolderType *holder) {
        return holder->user_data;
    }

    void InitializeMultiWaitHolder(MultiWaitHolderType *holder, NativeHandle handle) {
        AMS_ASSERT(handle != os::InvalidNativeHandle);

        util::ConstructAt(GetReference(holder->impl_storage).holder_of_handle_storage, handle);

        holder->user_data = 0;
    }

}
