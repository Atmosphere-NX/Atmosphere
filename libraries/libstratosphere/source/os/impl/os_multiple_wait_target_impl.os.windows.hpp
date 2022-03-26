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

    class MultiWaitWindowsImpl {
        public:
            static constexpr size_t MaximumHandleCount = static_cast<size_t>(MAXIMUM_WAIT_OBJECTS);
        private:
            os::NativeHandle m_cancel_event;
        private:
            Result WaitForMultipleObjectsImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, DWORD wait_ms);
            Result ReplyAndReceiveImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns, NativeHandle reply_target);
        public:
            MultiWaitWindowsImpl() {
                m_cancel_event = ::CreateEvent(nullptr, false, false, nullptr);
                AMS_ASSERT(m_cancel_event != os::InvalidNativeHandle);
            }

            ~MultiWaitWindowsImpl() {
                const auto ret = ::CloseHandle(m_cancel_event);
                AMS_ASSERT(ret != 0);
                AMS_UNUSED(ret);
            }

            void CancelWait() {
                ::SetEvent(m_cancel_event);
            }

            Result WaitAny(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num) {
                R_RETURN(this->WaitForMultipleObjectsImpl(out_index, num, arr, array_size, INFINITE));
            }

            Result TryWaitAny(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num) {
                R_RETURN(this->WaitForMultipleObjectsImpl(out_index, num, arr, array_size, 0));
            }

            Result TimedWaitAny(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num, TimeSpan ts);

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

    using MultiWaitTargetImpl = MultiWaitWindowsImpl;

}
