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
#include "os_inter_process_event.hpp"
#include "os_inter_process_event_impl.os.macos.hpp"
#include "os_timeout_helper.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

namespace ams::os::impl {

    namespace {

        /* On macOS, the maximum size of a pipe buffer is 64_KB. */
        static constexpr size_t PipeBufferSizeMax = 64_KB;

        constinit char g_shared_pipe_read_buffer[PipeBufferSizeMax];

        bool PollEvent(NativeHandle handle, s64 ns) {
            struct pollfd pfd;
            pfd.fd      = handle;
            pfd.events  = POLLIN;
            pfd.revents = 0;

            /* Determine timeout. */
            constexpr s64 NanoSecondsPerMilliSecond = TimeSpan::FromMilliSeconds(1).GetNanoSeconds();

            /* TODO: Will macos ever support ppoll? */
            const int timeout = static_cast<int>(ns >= 0 ? (ns / NanoSecondsPerMilliSecond) : -1);

            s32 res;
            do {
                res = ::poll(std::addressof(pfd), 1, timeout);
            } while (res < 0 && errno == EINTR);

            AMS_ASSERT(res == 0 || res == 1);

            const bool signaled = pfd.revents & POLLIN;
            AMS_ASSERT(signaled == (res == 1));
            return signaled;
        }

    }

    bool InterProcessEventMacosImpl::ResetEventSignal(NativeHandle handle) {
        s32 res;
        do {
            res = ::read(handle, g_shared_pipe_read_buffer, sizeof(g_shared_pipe_read_buffer));
        } while (res < 0 && errno == EINTR);

        if (res > 0) {
            AMS_ASSERT(res <= static_cast<s32>(sizeof(g_shared_pipe_read_buffer)));
        } else {
            AMS_ASSERT(res < 0);
            AMS_ASSERT(errno == EAGAIN);
        }

        return res > 0;
    }

    Result InterProcessEventMacosImpl::Create(NativeHandle *out_write, NativeHandle *out_read) {
        /* Create the handles. */
        os::NativeHandle handles[2];
        s32 res;
        do {
            res = ::pipe(handles);
        } while (res < 0 && errno == EINTR);
        R_UNLESS(res == 0, os::ResultOutOfResource());
        ON_RESULT_FAILURE { Close(handles[0]); Close(handles[1]); };

        /* Set as non-blocking. */
        do {
            res = ::fcntl(handles[0], F_SETFL, O_NONBLOCK);
        } while (res < 0 && errno == EINTR);
        R_UNLESS(res == 0, os::ResultOutOfResource());

        do {
            res = ::fcntl(handles[1], F_SETFL, O_NONBLOCK);
        } while (res < 0 && errno == EINTR);
        R_UNLESS(res == 0, os::ResultOutOfResource());

        /* Set the output. */
        *out_read  = handles[0];
        *out_write = handles[1];
        R_SUCCEED();
    }

    void InterProcessEventMacosImpl::Close(NativeHandle handle) {
        if (handle != os::InvalidNativeHandle) {
            s32 ret;
            do {
                ret = ::close(handle);
            } while (ret < 0 && errno == EINTR);
            AMS_ASSERT(ret == 0);
        }
    }

    void InterProcessEventMacosImpl::Signal(NativeHandle handle) {
        const u8 data = 0xCC;

        s32 ret;
        do {
            ret = ::write(handle, std::addressof(data), sizeof(data));
        } while (ret < 0 && errno == EINTR);
        AMS_ASSERT(ret == sizeof(data) || (ret < 0 && errno == EAGAIN));
    }

    void InterProcessEventMacosImpl::Clear(NativeHandle handle) {
        ResetEventSignal(handle);
    }

    void InterProcessEventMacosImpl::Wait(NativeHandle handle, bool auto_clear) {
        while (true) {
            /* Continuously wait, until success. */
            auto ret = PollEvent(handle, -1);
            AMS_ASSERT(ret);
            AMS_UNUSED(ret);

            /* If we're not obligated to clear, we're done. */
            if (!auto_clear) {
                return;
            }

            /* Try to reset. */
            if (ResetEventSignal(handle)) {
                return;
            }
        }
    }

    bool InterProcessEventMacosImpl::TryWait(NativeHandle handle, bool auto_clear) {
        /* If we're auto clear, just try to reset. */
        if (auto_clear) {
            return ResetEventSignal(handle);
        }

        return PollEvent(handle, 0);
    }

    bool InterProcessEventMacosImpl::TimedWait(NativeHandle handle, bool auto_clear, TimeSpan timeout) {
        TimeoutHelper timeout_helper(timeout);

        do {
            if (const auto res = PollEvent(handle, timeout_helper.GetTimeLeftOnTarget().GetNanoSeconds()); res) {
                /* If we're not obligated to clear, we're done. */
                if (!auto_clear) {
                    return true;
                }

                /* Try to reset. */
                if (ResetEventSignal(handle)) {
                    return true;
                }
            }
        } while (!timeout_helper.TimedOut());

        return false;
    }

}
