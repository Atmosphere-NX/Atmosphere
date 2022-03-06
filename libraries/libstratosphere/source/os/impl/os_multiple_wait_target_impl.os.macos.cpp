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
#include "os_inter_process_event_impl.os.macos.hpp"

#include <poll.h>

namespace ams::os::impl {

    MultiWaitMacosImpl::MultiWaitMacosImpl() {
        R_ABORT_UNLESS(InterProcessEventMacosImpl::Create(std::addressof(m_cancel_write_handle), std::addressof(m_cancel_read_handle)));
    }

    MultiWaitMacosImpl::~MultiWaitMacosImpl() {
        InterProcessEventMacosImpl::Close(m_cancel_write_handle);
        InterProcessEventMacosImpl::Close(m_cancel_read_handle);

        m_cancel_write_handle = InvalidNativeHandle;
        m_cancel_read_handle  = InvalidNativeHandle;
    }

    void MultiWaitMacosImpl::CancelWait() {
        InterProcessEventMacosImpl::Signal(m_cancel_write_handle);
    }

    Result MultiWaitMacosImpl::PollNativeHandlesImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns) {
        /* Check that we can add our cancel handle to the wait. */
        AMS_ABORT_UNLESS(array_size <= static_cast<s32>(MaximumHandleCount));
        AMS_UNUSED(array_size);

        /* Create poll fds. */
        struct pollfd pfds[MaximumHandleCount + 1];
        for (auto i = 0; i < num; ++i) {
            pfds[i].fd      = arr[i];
            pfds[i].events  = POLLIN;
            pfds[i].revents = 0;
        }

        pfds[num].fd      = m_cancel_read_handle;
        pfds[num].events  = POLLIN;
        pfds[num].revents = 0;

        /* Determine timeout. */
        constexpr s64 NanoSecondsPerMilliSecond = TimeSpan::FromMilliSeconds(1).GetNanoSeconds();

        /* TODO: Will macos ever support ppoll? */
        const int timeout = static_cast<int>(ns >= 0 ? (ns / NanoSecondsPerMilliSecond) : -1);

        /* Wait. */
        while (true) {
            const auto ret = ::poll(pfds, num + 1, timeout);
            if (ret < 0) {
                /* Treat EINTR like a cancellation event; this will lead to a re-poll if nothing is signaled. */
                AMS_ABORT_UNLESS(errno == EINTR);

                *out_index = MultiWaitImpl::WaitCancelled;
                R_SUCCEED();
            }

            /* Determine what event was polled. */
            if (ret == 0) {
                *out_index = MultiWaitImpl::WaitTimedOut;
                R_SUCCEED();
            } else if (pfds[num].revents != 0) {
                *out_index = MultiWaitImpl::WaitCancelled;

                /* Reset our cancel event. */
                InterProcessEventMacosImpl::Clear(m_cancel_read_handle);

                R_SUCCEED();
            } else {
                for (auto i = 0; i < num; ++i) {
                    if (pfds[i].revents != 0) {
                        *out_index = i;
                        R_SUCCEED();
                    }
                }

                AMS_ABORT("This should be impossible?");
            }
        }
    }

    Result MultiWaitMacosImpl::ReplyAndReceiveImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns, NativeHandle reply_target) {
        AMS_UNUSED(out_index, num, arr, array_size, ns, reply_target);
        R_ABORT_UNLESS(os::ResultNotImplemented());
    }

}
