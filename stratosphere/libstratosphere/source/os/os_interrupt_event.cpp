/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "impl/os_waitable_object_list.hpp"

namespace sts::os {

    Result InterruptEvent::Initialize(u32 interrupt_id, bool autoclear) {
        STS_ASSERT(!this->is_initialized);
        this->auto_clear = autoclear;

        const auto type = this->auto_clear ? svc::InterruptType_Edge : svc::InterruptType_Level;
        R_TRY(svcCreateInterruptEvent(this->handle.GetPointer(), interrupt_id, type));

        this->is_initialized = true;
        return ResultSuccess;
    }

    void InterruptEvent::Finalize() {
        STS_ASSERT(this->is_initialized);
        R_ASSERT(svcCloseHandle(this->handle.Move()));
        this->auto_clear = true;
        this->is_initialized = false;
    }

    InterruptEvent::InterruptEvent(u32 interrupt_id, bool autoclear) {
        this->is_initialized = false;
        R_ASSERT(this->Initialize(interrupt_id, autoclear));
    }

    void InterruptEvent::Reset() {
        R_ASSERT(svcClearEvent(this->handle.Get()));
    }

    void InterruptEvent::Wait() {
        STS_ASSERT(this->is_initialized);

        while (true) {
            /* Continuously wait, until success. */
            R_TRY_CATCH(svcWaitSynchronizationSingle(this->handle.Get(), U64_MAX)) {
                R_CATCH(ResultKernelCancelled) { continue; }
            } R_END_TRY_CATCH_WITH_ASSERT;

            /* Clear, if we must. */
            if (this->auto_clear) {
                R_TRY_CATCH(svcResetSignal(this->handle.Get())) {
                    /* Some other thread might have caught this before we did. */
                    R_CATCH(ResultKernelInvalidState) { continue; }
                } R_END_TRY_CATCH_WITH_ASSERT;
            }
            return;
        }
    }

    bool InterruptEvent::TryWait() {
        STS_ASSERT(this->is_initialized);

        if (this->auto_clear) {
            /* Auto-clear. Just try to reset. */
            return R_SUCCEEDED(svcResetSignal(this->handle.Get()));
        } else {
            /* Not auto-clear. */
            while (true) {
                /* Continuously wait, until success or timeout. */
                R_TRY_CATCH(svcWaitSynchronizationSingle(this->handle.Get(), 0)) {
                    R_CATCH(ResultKernelTimedOut) { return false; }
                    R_CATCH(ResultKernelCancelled) { continue; }
                } R_END_TRY_CATCH_WITH_ASSERT;

                /* We succeeded, so we're signaled. */
                return true;
            }
        }
    }

    bool InterruptEvent::TimedWait(u64 ns) {
        STS_ASSERT(this->is_initialized);

        TimeoutHelper timeout_helper(ns);
        while (true) {
            /* Continuously wait, until success or timeout. */
            R_TRY_CATCH(svcWaitSynchronizationSingle(this->handle.Get(), timeout_helper.NsUntilTimeout())) {
                R_CATCH(ResultKernelTimedOut) { return false; }
                R_CATCH(ResultKernelCancelled) { continue; }
            } R_END_TRY_CATCH_WITH_ASSERT;

            /* Clear, if we must. */
            if (this->auto_clear) {
                R_TRY_CATCH(svcResetSignal(this->handle.Get())) {
                    /* Some other thread might have caught this before we did. */
                    R_CATCH(ResultKernelInvalidState) { continue; }
                } R_END_TRY_CATCH_WITH_ASSERT;
            }

            return true;
        }
    }

}
