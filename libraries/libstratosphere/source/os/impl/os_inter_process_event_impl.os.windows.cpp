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
#include <stratosphere/windows.hpp>
#include "os_inter_process_event.hpp"
#include "os_inter_process_event_impl.os.windows.hpp"
#include "os_giant_lock.hpp"
#include "os_timeout_helper.hpp"

namespace ams::os::impl {

    bool InterProcessEventWindowsImpl::ResetEventSignal(NativeHandle handle) {
        std::scoped_lock lk(GetGiantLock());

        if (auto ret = ::WaitForSingleObject(handle, 0); ret == WAIT_OBJECT_0) {
            ::ResetEvent(handle);
            return true;
        } else {
            return false;
        }
    }

    Result InterProcessEventWindowsImpl::Create(NativeHandle *out_write, NativeHandle *out_read) {
        /* Create the writable handle. */
        auto write = ::CreateEvent(nullptr, true, false, nullptr);
        AMS_ASSERT(write != os::InvalidNativeHandle);

        /* Create the read handle. */
        os::NativeHandle read;
        const auto cur_proc = ::GetCurrentProcess();
        const auto ret = ::DuplicateHandle(cur_proc, write, cur_proc, std::addressof(read), 0, false, DUPLICATE_SAME_ACCESS);
        AMS_ASSERT(ret != 0);
        AMS_UNUSED(ret);

        /* Set the output. */
        *out_write = write;
        *out_read  = read;
        R_SUCCEED();
    }

    void InterProcessEventWindowsImpl::Close(NativeHandle handle) {
        if (handle != os::InvalidNativeHandle) {
            const auto ret = ::CloseHandle(handle);
            AMS_ASSERT(ret != 0);
            AMS_UNUSED(ret);
        }
    }

    void InterProcessEventWindowsImpl::Signal(NativeHandle handle) {
        const auto ret = ::SetEvent(handle);
        AMS_ASSERT(ret != 0);
        AMS_UNUSED(ret);
    }

    void InterProcessEventWindowsImpl::Clear(NativeHandle handle) {
        const auto ret = ::ResetEvent(handle);
        AMS_ASSERT(ret != 0);
        AMS_UNUSED(ret);
    }

    void InterProcessEventWindowsImpl::Wait(NativeHandle handle, bool auto_clear) {
        while (true) {
            /* Continuously wait, until success. */
            auto ret = ::WaitForSingleObject(handle, INFINITE);
            AMS_ASSERT(ret == WAIT_OBJECT_0);
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

    bool InterProcessEventWindowsImpl::TryWait(NativeHandle handle, bool auto_clear) {
        /* If we're auto clear, just try to reset. */
        if (auto_clear) {
            return ResetEventSignal(handle);
        }

        const auto ret = ::WaitForSingleObject(handle, 0);
        AMS_ASSERT((ret == WAIT_OBJECT_0) || (ret == WAIT_TIMEOUT));

        return ret == WAIT_OBJECT_0;
    }

    bool InterProcessEventWindowsImpl::TimedWait(NativeHandle handle, bool auto_clear, TimeSpan timeout) {
        TimeoutHelper timeout_helper(timeout);

        do {
            if (const auto res = ::WaitForSingleObject(handle, timeout_helper.GetTimeLeftOnTarget()); res == WAIT_OBJECT_0) {
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
