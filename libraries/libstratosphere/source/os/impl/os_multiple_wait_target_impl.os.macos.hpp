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
#include <stratosphere.hpp>
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    class MultiWaitMacosImpl {
        public:
            /* TODO: This can potentially be higher. */
            static constexpr size_t MaximumHandleCount = 64;
        private:
            NativeHandle m_cancel_read_handle;
            NativeHandle m_cancel_write_handle;
        private:
            Result PollNativeHandlesImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns);
            Result ReplyAndReceiveImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns, NativeHandle reply_target);
        public:
            MultiWaitMacosImpl();
            ~MultiWaitMacosImpl();

            void CancelWait();

            Result WaitAny(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num) {
                R_RETURN(this->PollNativeHandlesImpl(out_index, num, arr, array_size, static_cast<s64>(-1)));
            }

            Result TryWaitAny(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num) {
                R_RETURN(this->PollNativeHandlesImpl(out_index, num, arr, array_size, 0));
            }

            Result TimedWaitAny(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num, TimeSpan ts) {
                R_RETURN(this->PollNativeHandlesImpl(out_index, num, arr, array_size, ts.GetNanoSeconds()));
            }

            Result ReplyAndReceive(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num, NativeHandle reply_target) {
                R_RETURN(this->ReplyAndReceiveImpl(out_index, num, arr, array_size, std::numeric_limits<s64>::max(), reply_target));
            }

            Result TimedReplyAndReceive(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num, NativeHandle reply_target, TimeSpan ts) {
                R_RETURN(this->ReplyAndReceiveImpl(out_index, num, arr, array_size, ts.GetNanoSeconds(), reply_target));
            }

            void SetCurrentThreadHandleForCancelWait() {
                /* ... */
            }

            void ClearCurrentThreadHandleForCancelWait() {
                /* ... */
            }
    };

    using MultiWaitTargetImpl = MultiWaitMacosImpl;

}
