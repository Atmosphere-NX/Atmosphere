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
#include <stratosphere.hpp>
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    class WaitableManagerHorizonImpl {
        public:
            static constexpr size_t MaximumHandleCount = static_cast<size_t>(ams::svc::ArgumentHandleCountMax);
        private:
            Handle handle;
        private:
            Result WaitSynchronizationN(s32 *out_index, s32 num, Handle arr[], s32 array_size, s64 ns);
            Result ReplyAndReceiveN(s32 *out_index, s32 num, Handle arr[], s32 array_size, s64 ns, Handle reply_target);
        public:
            void CancelWait();

            Result WaitAny(s32 *out_index, Handle arr[], s32 array_size, s32 num) {
                return this->WaitSynchronizationN(out_index, num, arr, array_size, svc::WaitInfinite);
            }

            Result TryWaitAny(s32 *out_index, Handle arr[], s32 array_size, s32 num) {
                return this->WaitSynchronizationN(out_index, num, arr, array_size, 0);
            }

            Result TimedWaitAny(s32 *out_index, Handle arr[], s32 array_size, s32 num, TimeSpan ts) {
                s64 timeout = ts.GetNanoSeconds();
                if (timeout < 0) {
                    timeout = 0;
                }
                return this->WaitSynchronizationN(out_index, num, arr, array_size, timeout);
            }

            Result ReplyAndReceive(s32 *out_index, Handle arr[], s32 array_size, s32 num, Handle reply_target) {
                return this->ReplyAndReceiveN(out_index, num, arr, array_size, std::numeric_limits<s64>::max(), reply_target);
            }

            Result TimedReplyAndReceive(s32 *out_index, Handle arr[], s32 array_size, s32 num, Handle reply_target, TimeSpan ts) {
                return this->ReplyAndReceiveN(out_index, num, arr, array_size, ts.GetNanoSeconds(), reply_target);
            }

            void SetCurrentThreadHandleForCancelWait() {
                this->handle = GetCurrentThreadHandle();
            }

            void ClearCurrentThreadHandleForCancelWait() {
                this->handle = svc::InvalidHandle;
            }
    };

    using WaitableManagerTargetImpl = WaitableManagerHorizonImpl;

}
