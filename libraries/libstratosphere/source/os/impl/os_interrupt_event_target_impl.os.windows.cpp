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
#include "os_interrupt_event_target_impl.os.windows.hpp"
#include "os_timeout_helper.hpp"
#include "os_giant_lock.hpp"

namespace ams::os::impl {

    InterruptEventWindowsImpl::InterruptEventWindowsImpl(InterruptName name, EventClearMode clear_mode) {
        const NativeHandle handle = ::OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, reinterpret_cast<LPCTSTR>(name));
        AMS_ASSERT(handle != InvalidNativeHandle);

        m_handle       = handle;
        m_manual_clear = (clear_mode == EventClearMode_ManualClear);
    }

    InterruptEventWindowsImpl::~InterruptEventWindowsImpl() {
        const auto ret = ::CloseHandle(m_handle);
        AMS_ASSERT(ret != 0);
        AMS_UNUSED(ret);
    }

    bool InterruptEventWindowsImpl::ResetEventSignal() {
        std::scoped_lock lk(GetGiantLock());

        if (auto ret = ::WaitForSingleObject(m_handle, 0); ret == WAIT_OBJECT_0) {
            ::ResetEvent(m_handle);
            return true;
        } else {
            return false;
        }
    }

    void InterruptEventWindowsImpl::Clear() {
        const auto ret = ::ResetEvent(m_handle);
        AMS_ASSERT(ret != 0);
        AMS_UNUSED(ret);
    }

    void InterruptEventWindowsImpl::Wait() {
        while (true) {
            /* Continuously wait, until success. */
            auto ret = ::WaitForSingleObject(m_handle, INFINITE);
            AMS_ASSERT(ret == WAIT_OBJECT_0);
            AMS_UNUSED(ret);

            /* If we're not obligated to clear, we're done. */
            if (m_manual_clear) {
                return;
            }

            /* Try to reset. */
            if (this->ResetEventSignal()) {
                return;
            }
        }
    }

    bool InterruptEventWindowsImpl::TryWait() {
        /* If we're auto clear, just try to reset. */
        if (!m_manual_clear) {
            return this->ResetEventSignal();
        }

        const auto ret = ::WaitForSingleObject(m_handle, 0);
        AMS_ASSERT((ret == WAIT_OBJECT_0) || (ret == WAIT_TIMEOUT));

        return ret == WAIT_OBJECT_0;
    }

    bool InterruptEventWindowsImpl::TimedWait(TimeSpan timeout) {
        TimeoutHelper timeout_helper(timeout);

        do {
            if (const auto res = ::WaitForSingleObject(m_handle, timeout_helper.GetTimeLeftOnTarget()); res == WAIT_OBJECT_0) {
                /* If we're not obligated to clear, we're done. */
                if (m_manual_clear) {
                    return true;
                }

                /* Try to reset. */
                if (this->ResetEventSignal()) {
                    return true;
                }
            }
        } while (!timeout_helper.TimedOut());

        return false;
    }

}
