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

    void KernelDebug64(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
        MESOSPHERE_PANIC("Stubbed SvcKernelDebug64 was called.");
    }

    void ChangeKernelTraceState64(ams::svc::KernelTraceState kern_trace_state) {
        MESOSPHERE_PANIC("Stubbed SvcChangeKernelTraceState64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    void KernelDebug64From32(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
        MESOSPHERE_PANIC("Stubbed SvcKernelDebug64From32 was called.");
    }

    void ChangeKernelTraceState64From32(ams::svc::KernelTraceState kern_trace_state) {
        MESOSPHERE_PANIC("Stubbed SvcChangeKernelTraceState64From32 was called.");
    }

}
