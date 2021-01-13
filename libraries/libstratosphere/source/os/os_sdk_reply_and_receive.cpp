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

    Result SdkReplyAndReceive(os::WaitableHolderType **out, Handle reply_target, WaitableManagerType *manager) {
        auto &impl = GetWaitableManagerImpl(manager);

        AMS_ASSERT(manager->state == WaitableManagerType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());

        impl::WaitableHolderBase *holder_base;
        ON_SCOPE_EXIT { *out = CastToWaitableHolder(holder_base); };

        return impl.ReplyAndReceive(std::addressof(holder_base), reply_target);
    }

}
