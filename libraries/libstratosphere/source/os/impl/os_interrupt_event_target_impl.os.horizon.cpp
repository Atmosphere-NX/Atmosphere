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
#include <stratosphere.hpp>
#include "os_interrupt_event_target_impl.os.horizon.hpp"
#include "os_timeout_helper.hpp"

namespace ams::os::impl {

    InterruptEventHorizonImpl::InterruptEventHorizonImpl(InterruptName name, EventClearMode clear_mode) {
        this->manual_clear = (clear_mode == EventClearMode_ManualClear);

        auto interrupt_type = this->manual_clear ? svc::InterruptType_Level : svc::InterruptType_Edge;
        svc::Handle handle;
        R_ABORT_UNLESS(svc::CreateInterruptEvent(std::addressof(handle), static_cast<s32>(name), interrupt_type));

        this->handle = handle;
    }

    InterruptEventHorizonImpl::~InterruptEventHorizonImpl() {
        R_ABORT_UNLESS(svc::CloseHandle(this->handle));
    }

    void InterruptEventHorizonImpl::Clear() {
        R_ABORT_UNLESS(svc::ClearEvent(this->handle));
    }

    void InterruptEventHorizonImpl::Wait() {
        while (true) {
            /* Continuously wait, until success. */
            s32 index;
            Result res = svc::WaitSynchronization(std::addressof(index), std::addressof(this->handle), 1, svc::WaitInfinite);
            if (R_SUCCEEDED(res)) {
                /* Clear, if we must. */
                if (!this->manual_clear) {
                    R_TRY_CATCH(svc::ResetSignal(this->handle)) {
                        /* Some other thread might have caught this before we did. */
                        R_CATCH(svc::ResultInvalidState) { continue; }
                    } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                }

                return;
            }

            AMS_ASSERT(svc::ResultCancelled::Includes(res));
        }
    }

    bool InterruptEventHorizonImpl::TryWait() {
        /* If we're auto clear, just try to reset. */
        if (!this->manual_clear) {
            return R_SUCCEEDED(svc::ResetSignal(this->handle));
        }

        /* Not auto-clear. */
        while (true) {
            /* Continuously wait, until success or timeout. */
            s32 index;
            Result res = svc::WaitSynchronization(std::addressof(index), std::addressof(this->handle), 1, 0);

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

    bool InterruptEventHorizonImpl::TimedWait(TimeSpan timeout) {
        TimeoutHelper timeout_helper(timeout);

        while (true) {
            /* Continuously wait, until success. */
            s32 index;
            Result res = svc::WaitSynchronization(std::addressof(index), std::addressof(this->handle), 1, timeout_helper.GetTimeLeftOnTarget().GetNanoSeconds());
            if (R_SUCCEEDED(res)) {
                /* Clear, if we must. */
                if (!this->manual_clear) {
                    R_TRY_CATCH(svc::ResetSignal(this->handle)) {
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
