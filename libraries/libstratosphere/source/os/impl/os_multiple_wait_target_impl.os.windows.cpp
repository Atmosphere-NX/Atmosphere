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
#include "os_multiple_wait_holder_base.hpp"
#include "os_multiple_wait_impl.hpp"
#include "os_timeout_helper.hpp"

namespace ams::os::impl {

    Result MultiWaitWindowsImpl::WaitForMultipleObjectsImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, DWORD wait_ms) {
        /* Check that we can add our cancel handle to the wait. */
        AMS_ABORT_UNLESS(num + 1 <= array_size);
        AMS_UNUSED(array_size);

        /* Add our cancel handle. */
        arr[num] = m_cancel_event;

        /* Wait. */
        auto result = ::WaitForMultipleObjects(num + 1, arr, FALSE, wait_ms);
        if (result == WAIT_TIMEOUT) {
            *out_index = MultiWaitImpl::WaitTimedOut;
        } else {
            const s32 index = result - WAIT_OBJECT_0;
            AMS_ASSERT((0 <= index) && (index <= num));
            if (index == num) {
                *out_index = MultiWaitImpl::WaitCancelled;
            } else {
                *out_index = index;
            }
        }

        R_SUCCEED();
    }

    Result MultiWaitWindowsImpl::TimedWaitAny(s32 *out_index, NativeHandle arr[], s32 array_size, s32 num, TimeSpan ts) {
        impl::TimeoutHelper timeout(ts);

        do {
            s32 idx;
            R_TRY(WaitForMultipleObjectsImpl(std::addressof(idx), num, arr, array_size, timeout.GetTimeLeftOnTarget()));
            if (idx != MultiWaitImpl::WaitTimedOut) {
                *out_index = idx;
                R_SUCCEED();
            }
        } while (!timeout.TimedOut());

        *out_index = MultiWaitImpl::WaitTimedOut;
        R_SUCCEED();
    }

    Result MultiWaitWindowsImpl::ReplyAndReceiveImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns, NativeHandle reply_target) {
        AMS_UNUSED(out_index, num, arr, array_size, ns, reply_target);
        R_ABORT_UNLESS(os::ResultNotImplemented());
    }

}
