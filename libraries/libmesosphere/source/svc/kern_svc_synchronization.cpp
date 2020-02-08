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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {



    }

    /* =============================    64 ABI    ============================= */

    Result CloseHandle64(ams::svc::Handle handle) {
        MESOSPHERE_PANIC("Stubbed SvcCloseHandle64 was called.");
    }

    Result ResetSignal64(ams::svc::Handle handle) {
        MESOSPHERE_PANIC("Stubbed SvcResetSignal64 was called.");
    }

    Result WaitSynchronization64(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t numHandles, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcWaitSynchronization64 was called.");
    }

    Result CancelSynchronization64(ams::svc::Handle handle) {
        MESOSPHERE_PANIC("Stubbed SvcCancelSynchronization64 was called.");
    }

    void SynchronizePreemptionState64() {
        MESOSPHERE_PANIC("Stubbed SvcSynchronizePreemptionState64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result CloseHandle64From32(ams::svc::Handle handle) {
        MESOSPHERE_PANIC("Stubbed SvcCloseHandle64From32 was called.");
    }

    Result ResetSignal64From32(ams::svc::Handle handle) {
        MESOSPHERE_PANIC("Stubbed SvcResetSignal64From32 was called.");
    }

    Result WaitSynchronization64From32(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t numHandles, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcWaitSynchronization64From32 was called.");
    }

    Result CancelSynchronization64From32(ams::svc::Handle handle) {
        MESOSPHERE_PANIC("Stubbed SvcCancelSynchronization64From32 was called.");
    }

    void SynchronizePreemptionState64From32() {
        MESOSPHERE_PANIC("Stubbed SvcSynchronizePreemptionState64From32 was called.");
    }

}
