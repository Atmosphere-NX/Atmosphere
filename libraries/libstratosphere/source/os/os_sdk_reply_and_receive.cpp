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

    Result SdkReplyAndReceive(os::MultiWaitHolderType **out, NativeHandle reply_target, MultiWaitType *multi_wait) {
        auto &impl = GetMultiWaitImpl(multi_wait);

        AMS_ASSERT(multi_wait->state == MultiWaitType::State_Initialized);
        AMS_ASSERT(!impl.IsEmpty());

        impl::MultiWaitHolderBase *holder_base;
        ON_SCOPE_EXIT { *out = CastToMultiWaitHolder(holder_base); };

        return impl.ReplyAndReceive(std::addressof(holder_base), reply_target);
    }

}
