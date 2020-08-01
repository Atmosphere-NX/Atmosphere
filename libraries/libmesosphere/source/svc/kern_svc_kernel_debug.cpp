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

        void KernelDebug(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
            #ifdef ATMOSPHERE_BUILD_FOR_DEBUGGING
            {
                /* TODO: Implement Kernel Debugging. */
            }
            #endif
        }

        void ChangeKernelTraceState(ams::svc::KernelTraceState kern_trace_state) {
            #ifdef ATMOSPHERE_BUILD_FOR_DEBUGGING
            {
                switch (kern_trace_state) {
                    case ams::svc::KernelTraceState_Enabled:
                        {
                            MESOSPHERE_KTRACE_RESUME();
                        }
                        break;
                    case ams::svc::KernelTraceState_Disabled:
                        {
                            MESOSPHERE_KTRACE_PAUSE();
                        }
                        break;
                    default:
                        break;
                }
            }
            #endif
        }

    }

    /* =============================    64 ABI    ============================= */

    void KernelDebug64(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
        return KernelDebug(kern_debug_type, arg0, arg1, arg2);
    }

    void ChangeKernelTraceState64(ams::svc::KernelTraceState kern_trace_state) {
        return ChangeKernelTraceState(kern_trace_state);
    }

    /* ============================= 64From32 ABI ============================= */

    void KernelDebug64From32(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
        return KernelDebug(kern_debug_type, arg0, arg1, arg2);
    }

    void ChangeKernelTraceState64From32(ams::svc::KernelTraceState kern_trace_state) {
        return ChangeKernelTraceState(kern_trace_state);
    }

}
