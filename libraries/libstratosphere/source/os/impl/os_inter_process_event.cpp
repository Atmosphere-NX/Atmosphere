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
#include "os_inter_process_event.hpp"

namespace ams::os::impl {

    namespace {

        Result CreateEventHandles(Handle *out_readable, Handle *out_writable) {
            /* Create the event handles. */
            R_TRY_CATCH(svcCreateEvent(out_writable, out_readable)) {
                R_CONVERT(svc::ResultOutOfResource, ResultOutOfResource());
            } R_END_TRY_CATCH_WITH_ASSERT;

            return ResultSuccess();
        }

    }

    InterProcessEvent::InterProcessEvent(bool autoclear) : is_initialized(false) {
        R_ASSERT(this->Initialize(autoclear));
    }

    InterProcessEvent::~InterProcessEvent() {
        this->Finalize();
    }

    Result InterProcessEvent::Initialize(bool autoclear) {
        AMS_ASSERT(!this->is_initialized);
        Handle rh, wh;
        R_TRY(CreateEventHandles(&rh, &wh));
        this->Initialize(rh, true, wh, true, autoclear);
        return ResultSuccess();
    }

    void InterProcessEvent::Initialize(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, bool autoclear) {
        AMS_ASSERT(!this->is_initialized);
        AMS_ASSERT(read_handle != INVALID_HANDLE || write_handle != INVALID_HANDLE);
        this->read_handle = read_handle;
        this->manage_read_handle = manage_read_handle;
        this->write_handle = write_handle;
        this->manage_write_handle = manage_write_handle;
        this->auto_clear = autoclear;
        this->is_initialized = true;
    }

    Handle InterProcessEvent::DetachReadableHandle() {
        AMS_ASSERT(this->is_initialized);
        const Handle handle = this->read_handle;
        AMS_ASSERT(handle != INVALID_HANDLE);
        this->read_handle = INVALID_HANDLE;
        this->manage_read_handle = false;
        return handle;
    }

    Handle InterProcessEvent::DetachWritableHandle() {
        AMS_ASSERT(this->is_initialized);
        const Handle handle = this->write_handle;
        AMS_ASSERT(handle != INVALID_HANDLE);
        this->write_handle = INVALID_HANDLE;
        this->manage_write_handle = false;
        return handle;
    }

    Handle InterProcessEvent::GetReadableHandle() const {
        AMS_ASSERT(this->is_initialized);
        return this->read_handle;
    }

    Handle InterProcessEvent::GetWritableHandle() const {
        AMS_ASSERT(this->is_initialized);
        return this->write_handle;
    }

    void InterProcessEvent::Finalize() {
        if (this->is_initialized) {
            if (this->manage_read_handle && this->read_handle != INVALID_HANDLE) {
                R_ASSERT(svcCloseHandle(this->read_handle));
            }
            if (this->manage_write_handle && this->write_handle != INVALID_HANDLE) {
                R_ASSERT(svcCloseHandle(this->write_handle));
            }
        }
        this->read_handle  = INVALID_HANDLE;
        this->manage_read_handle = false;
        this->write_handle = INVALID_HANDLE;
        this->manage_write_handle = false;
        this->is_initialized = false;
    }

    void InterProcessEvent::Signal() {
        R_ASSERT(svcSignalEvent(this->GetWritableHandle()));
    }

    void InterProcessEvent::Reset() {
        Handle handle = this->GetReadableHandle();
        if (handle == INVALID_HANDLE) {
            handle = this->GetWritableHandle();
        }
        R_ASSERT(svcClearEvent(handle));
    }

    void InterProcessEvent::Wait() {
        const Handle handle = this->GetReadableHandle();

        while (true) {
            /* Continuously wait, until success. */
            R_TRY_CATCH(svcWaitSynchronizationSingle(handle, U64_MAX)) {
                R_CATCH(svc::ResultCancelled) { continue; }
            } R_END_TRY_CATCH_WITH_ASSERT;

            /* Clear, if we must. */
            if (this->auto_clear) {
                R_TRY_CATCH(svcResetSignal(handle)) {
                    /* Some other thread might have caught this before we did. */
                    R_CATCH(svc::ResultInvalidState) { continue; }
                } R_END_TRY_CATCH_WITH_ASSERT;
            }
            return;
        }
    }

    bool InterProcessEvent::TryWait() {
        const Handle handle = this->GetReadableHandle();

        if (this->auto_clear) {
            /* Auto-clear. Just try to reset. */
            return R_SUCCEEDED(svcResetSignal(handle));
        } else {
            /* Not auto-clear. */
            while (true) {
                /* Continuously wait, until success or timeout. */
                R_TRY_CATCH(svcWaitSynchronizationSingle(handle, 0)) {
                    R_CATCH(svc::ResultTimedOut) { return false; }
                    R_CATCH(svc::ResultCancelled) { continue; }
                } R_END_TRY_CATCH_WITH_ASSERT;

                /* We succeeded, so we're signaled. */
                return true;
            }
        }
    }

    bool InterProcessEvent::TimedWait(u64 ns) {
        const Handle handle = this->GetReadableHandle();

        TimeoutHelper timeout_helper(ns);
        while (true) {
            /* Continuously wait, until success or timeout. */
            R_TRY_CATCH(svcWaitSynchronizationSingle(handle, timeout_helper.NsUntilTimeout())) {
                R_CATCH(svc::ResultTimedOut) { return false; }
                R_CATCH(svc::ResultCancelled) { continue; }
            } R_END_TRY_CATCH_WITH_ASSERT;

            /* Clear, if we must. */
            if (this->auto_clear) {
                R_TRY_CATCH(svcResetSignal(handle)) {
                    /* Some other thread might have caught this before we did. */
                    R_CATCH(svc::ResultInvalidState) { continue; }
                } R_END_TRY_CATCH_WITH_ASSERT;
            }

            return true;
        }
    }
}
