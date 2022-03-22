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
#include "os_inter_process_event_impl.os.horizon.hpp"
#include "os_timeout_helper.hpp"

namespace ams::os::impl {

    Result InterProcessEventImpl::Create(NativeHandle *out_write, NativeHandle *out_read) {
        /* Create the event handles. */
        svc::Handle wh, rh;
        R_TRY_CATCH(svc::CreateEvent(std::addressof(wh), std::addressof(rh))) {
            R_CONVERT(svc::ResultOutOfResource, os::ResultOutOfResource())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        *out_write = wh;
        *out_read  = rh;
        return ResultSuccess();
    }

    void InterProcessEventImpl::Close(NativeHandle handle) {
        if (handle != os::InvalidNativeHandle) {
            R_ABORT_UNLESS(svc::CloseHandle(handle));
        }
    }

    void InterProcessEventImpl::Signal(NativeHandle handle) {
        R_ABORT_UNLESS(svc::SignalEvent(handle));
    }

    void InterProcessEventImpl::Clear(NativeHandle handle) {
        R_ABORT_UNLESS(svc::ClearEvent(handle));
    }

    void InterProcessEventImpl::Wait(NativeHandle handle, bool auto_clear) {
        while (true) {
            /* Continuously wait, until success. */
            s32 index;
            Result res = svc::WaitSynchronization(std::addressof(index), static_cast<svc::Handle *>(std::addressof(handle)), 1, svc::WaitInfinite);
            if (R_SUCCEEDED(res)) {
                /* Clear, if we must. */
                if (auto_clear) {
                    R_TRY_CATCH(svc::ResetSignal(handle)) {
                        /* Some other thread might have caught this before we did. */
                        R_CATCH(svc::ResultInvalidState) { continue; }
                    } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                }

                return;
            }

            AMS_ASSERT(svc::ResultCancelled::Includes(res));
        }
    }

    bool InterProcessEventImpl::TryWait(NativeHandle handle, bool auto_clear) {
        /* If we're auto clear, just try to reset. */
        if (auto_clear) {
            return R_SUCCEEDED(svc::ResetSignal(handle));
        }

        /* Not auto-clear. */
        while (true) {
            /* Continuously wait, until success or timeout. */
            s32 index;
            Result res = svc::WaitSynchronization(std::addressof(index), static_cast<svc::Handle *>(std::addressof(handle)), 1, 0);

            /* If we succeeded, we're signaled. */
            if (R_SUCCEEDED(res)) {
                return true;
            }

            /* If we timed out, we're not signaled. */
            if (svc::ResultTimedOut::Includes(res)) {
                return false;
            }

            AMS_ASSERT(svc::ResultCancelled::Includes(res));
        }
    }

    bool InterProcessEventImpl::TimedWait(NativeHandle handle, bool auto_clear, TimeSpan timeout) {
        TimeoutHelper timeout_helper(timeout);

        while (true) {
            /* Continuously wait, until success. */
            s32 index;
            Result res = svc::WaitSynchronization(std::addressof(index), static_cast<svc::Handle *>(std::addressof(handle)), 1, timeout_helper.GetTimeLeftOnTarget().GetNanoSeconds());
            if (R_SUCCEEDED(res)) {
                /* Clear, if we must. */
                if (auto_clear) {
                    R_TRY_CATCH(svc::ResetSignal(handle)) {
                        /* Some other thread might have caught this before we did. */
                        R_CATCH(svc::ResultInvalidState) { continue; }
                    } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                }

                return true;
            }

            if (svc::ResultTimedOut::Includes(res)) {
                return false;
            }

            AMS_ASSERT(svc::ResultCancelled::Includes(res));
        }
    }

}
