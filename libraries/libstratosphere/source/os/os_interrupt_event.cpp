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
#include "impl/os_waitable_object_list.hpp"

namespace ams::os {

    Result InterruptEvent::Initialize(u32 interrupt_id, bool autoclear) {
        AMS_ABORT_UNLESS(!this->is_initialized);
        this->auto_clear = autoclear;

        const auto type = this->auto_clear ? svc::InterruptType_Edge : svc::InterruptType_Level;
        R_TRY(svcCreateInterruptEvent(this->handle.GetPointer(), interrupt_id, type));

        this->is_initialized = true;
        return ResultSuccess();
    }

    void InterruptEvent::Finalize() {
        AMS_ABORT_UNLESS(this->is_initialized);
        R_ABORT_UNLESS(svcCloseHandle(this->handle.Move()));
        this->auto_clear = true;
        this->is_initialized = false;
    }

    InterruptEvent::InterruptEvent(u32 interrupt_id, bool autoclear) {
        this->is_initialized = false;
        R_ABORT_UNLESS(this->Initialize(interrupt_id, autoclear));
    }

    void InterruptEvent::Reset() {
        R_ABORT_UNLESS(svcClearEvent(this->handle.Get()));
    }

    void InterruptEvent::Wait() {
        AMS_ABORT_UNLESS(this->is_initialized);

        while (true) {
            /* Continuously wait, until success. */
            R_TRY_CATCH(svcWaitSynchronizationSingle(this->handle.Get(), std::numeric_limits<u64>::max())) {
                R_CATCH(svc::ResultCancelled) { continue; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            /* Clear, if we must. */
            if (this->auto_clear) {
                R_TRY_CATCH(svcResetSignal(this->handle.Get())) {
                    /* Some other thread might have caught this before we did. */
                    R_CATCH(svc::ResultInvalidState) { continue; }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
            }
            return;
        }
    }

    bool InterruptEvent::TryWait() {
        AMS_ABORT_UNLESS(this->is_initialized);

        if (this->auto_clear) {
            /* Auto-clear. Just try to reset. */
            return R_SUCCEEDED(svcResetSignal(this->handle.Get()));
        } else {
            /* Not auto-clear. */
            while (true) {
                /* Continuously wait, until success or timeout. */
                R_TRY_CATCH(svcWaitSynchronizationSingle(this->handle.Get(), 0)) {
                    R_CATCH(svc::ResultTimedOut) { return false; }
                    R_CATCH(svc::ResultCancelled) { continue; }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                /* We succeeded, so we're signaled. */
                return true;
            }
        }
    }

    bool InterruptEvent::TimedWait(u64 ns) {
        AMS_ABORT_UNLESS(this->is_initialized);

        TimeoutHelper timeout_helper(ns);
        while (true) {
            /* Continuously wait, until success or timeout. */
            R_TRY_CATCH(svcWaitSynchronizationSingle(this->handle.Get(), timeout_helper.NsUntilTimeout())) {
                R_CATCH(svc::ResultTimedOut) { return false; }
                R_CATCH(svc::ResultCancelled) { continue; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            /* Clear, if we must. */
            if (this->auto_clear) {
                R_TRY_CATCH(svcResetSignal(this->handle.Get())) {
                    /* Some other thread might have caught this before we did. */
                    R_CATCH(svc::ResultInvalidState) { continue; }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
            }

            return true;
        }
    }

}
