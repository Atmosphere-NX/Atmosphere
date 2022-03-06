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
#include "os_inter_process_event_impl.os.linux.hpp"
#include "os_timeout_helper.hpp"

#include <unistd.h>
#include <sys/eventfd.h>
#include <poll.h>

namespace ams::os::impl {

    namespace {

        bool PollEvent(NativeHandle handle, s64 ns) {
            struct pollfd pfd;
            pfd.fd      = handle;
            pfd.events  = POLLIN;
            pfd.revents = 0;

            /* Determine timeout. */
            constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();
            struct timespec ts = { .tv_sec = (ns / NanoSecondsPerSecond), .tv_nsec = ns % NanoSecondsPerSecond };

            s32 res;
            do {
                res = ::ppoll(std::addressof(pfd), 1, ns >= 0 ? std::addressof(ts) : nullptr, nullptr);
            } while (res < 0 && errno == EINTR);

            AMS_ASSERT(res == 0 || res == 1);

            const bool signaled = pfd.revents & POLLIN;
            AMS_ASSERT(signaled == (res == 1));
            return signaled;
        }

    }

    bool InterProcessEventLinuxImpl::ResetEventSignal(NativeHandle handle) {
        u64 dummy;
        s32 res;
        do {
            res = ::read(handle, std::addressof(dummy), sizeof(dummy));
        } while (res < 0 && errno == EINTR);

        AMS_ASSERT(res == sizeof(u64) || (res < 0 && errno == EAGAIN));
        if (res == sizeof(u64)) {
            AMS_ASSERT(dummy > 0);
        }

        return res == sizeof(u64);
    }

    Result InterProcessEventLinuxImpl::CreateSingle(NativeHandle *out_event) {
        /* Create the event handle. */
        os::NativeHandle event;
        do {
            event = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        } while (event < 0 && errno == EINTR);
        R_UNLESS(event != os::InvalidNativeHandle, os::ResultOutOfResource());

        /* Set the output. */
        *out_event = event;
        R_SUCCEED();
    }

    Result InterProcessEventLinuxImpl::Create(NativeHandle *out_write, NativeHandle *out_read) {
        /* Create the writable handle. */
        os::NativeHandle write;
        R_TRY(CreateSingle(std::addressof(write)));
        ON_RESULT_FAILURE { Close(write); };

        /* Create the read handle. */
        os::NativeHandle read;
        do {
            read = ::dup(write);
        } while (read < 0 && errno == EINTR);
        R_UNLESS(read != os::InvalidNativeHandle, os::ResultOutOfResource());

        /* Set the output. */
        *out_write = write;
        *out_read  = read;
        R_SUCCEED();
    }

    void InterProcessEventLinuxImpl::Close(NativeHandle handle) {
        if (handle != os::InvalidNativeHandle) {
            s32 ret;
            do {
                ret = ::close(handle);
            } while (ret < 0 && errno == EINTR);
            AMS_ASSERT(ret == 0);
        }
    }

    void InterProcessEventLinuxImpl::Signal(NativeHandle handle) {
        const u64 counter_add = 1;

        s32 ret;
        do {
            ret = ::write(handle, std::addressof(counter_add), sizeof(counter_add));
        } while (ret < 0 && errno == EINTR);
        AMS_ASSERT(ret == sizeof(counter_add));
    }

    void InterProcessEventLinuxImpl::Clear(NativeHandle handle) {
        ResetEventSignal(handle);
    }

    void InterProcessEventLinuxImpl::Wait(NativeHandle handle, bool auto_clear) {
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

    bool InterProcessEventLinuxImpl::TryWait(NativeHandle handle, bool auto_clear) {
        /* If we're auto clear, just try to reset. */
        if (auto_clear) {
            return ResetEventSignal(handle);
        }

        return PollEvent(handle, 0);
    }

    bool InterProcessEventLinuxImpl::TimedWait(NativeHandle handle, bool auto_clear, TimeSpan timeout) {
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
