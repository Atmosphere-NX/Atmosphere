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
#include "os_inter_process_event_impl.os.linux.hpp"

#include <poll.h>

namespace ams::os::impl {

    MultiWaitLinuxImpl::MultiWaitLinuxImpl() {
        R_ABORT_UNLESS(InterProcessEventLinuxImpl::CreateSingle(std::addressof(m_cancel_event)));
    }

    MultiWaitLinuxImpl::~MultiWaitLinuxImpl() {
        InterProcessEventLinuxImpl::Close(m_cancel_event);

        m_cancel_event = InvalidNativeHandle;
    }

    void MultiWaitLinuxImpl::CancelWait() {
        InterProcessEventLinuxImpl::Signal(m_cancel_event);
    }

    Result MultiWaitLinuxImpl::PollNativeHandlesImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns) {
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

        pfds[num].fd      = m_cancel_event;
        pfds[num].events  = POLLIN;
        pfds[num].revents = 0;

        /* Determine timeout. */
        constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();
        struct timespec ts = { .tv_sec = (ns / NanoSecondsPerSecond), .tv_nsec = ns % NanoSecondsPerSecond };

        /* Wait. */
        while (true) {
            const auto ret = ::ppoll(pfds, num + 1, ns >= 0 ? std::addressof(ts) : nullptr, nullptr);
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
                InterProcessEventLinuxImpl::Clear(m_cancel_event);

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

    Result MultiWaitLinuxImpl::ReplyAndReceiveImpl(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns, NativeHandle reply_target) {
        AMS_UNUSED(out_index, num, arr, array_size, ns, reply_target);
        R_ABORT_UNLESS(os::ResultNotImplemented());
    }

}
