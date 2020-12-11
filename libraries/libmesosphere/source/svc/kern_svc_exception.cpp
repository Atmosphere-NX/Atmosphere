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

        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        void PrintBreak(ams::svc::BreakReason break_reason) {
            /* Print that break was called. */
            MESOSPHERE_RELEASE_LOG("%s: svc::Break(%d) was called, pid=%ld, tid=%ld\n", GetCurrentProcess().GetName(), static_cast<s32>(break_reason), GetCurrentProcess().GetId(), GetCurrentThread().GetId());

            /* Print the current thread's registers. */
            KDebug::PrintRegister();

            /* Print a backtrace. */
            KDebug::PrintBacktrace();
        }
        #endif

        void Break(ams::svc::BreakReason break_reason, uintptr_t address, size_t size) {
            /* Determine whether the break is only a notification. */
            const bool is_notification = (break_reason & ams::svc::BreakReason_NotificationOnlyFlag) != 0;

            /* If the break isn't a notification, print it. */
            if (!is_notification) {
                #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
                PrintBreak(break_reason);
                #endif
            }

            /* If the current process is attached to debugger, try to notify it. */
            if (GetCurrentProcess().IsAttachedToDebugger()) {
                if (R_SUCCEEDED(KDebug::BreakIfAttached(break_reason, address, size))) {
                    /* If we attached, set the pc to the instruction before the current one and return. */
                    KDebug::SetPreviousProgramCounter();
                    return;
                }
            }

            /* If the break is only a notification, we're done. */
            if (is_notification) {
                return;
            }

            /* Print that break was called. */
            MESOSPHERE_RELEASE_LOG("Break() called. %016lx\n", GetCurrentProcess().GetProgramId());

            /* Try to enter JIT debug state. */
            if (GetCurrentProcess().EnterJitDebug(ams::svc::DebugEvent_Exception, ams::svc::DebugException_UserBreak, KDebug::GetProgramCounter(GetCurrentThread()), break_reason, address, size)) {
                /* We entered JIT debug, so set the pc to the instruction before the current one and return. */
                KDebug::SetPreviousProgramCounter();
                return;
            }

            /* Exit the current process. */
            GetCurrentProcess().Exit();
        }

    }

    /* =============================    64 ABI    ============================= */

    void Break64(ams::svc::BreakReason break_reason, ams::svc::Address arg, ams::svc::Size size) {
        return Break(break_reason, arg, size);
    }

    /* ============================= 64From32 ABI ============================= */

    void Break64From32(ams::svc::BreakReason break_reason, ams::svc::Address arg, ams::svc::Size size) {
        return Break(break_reason, arg, size);
    }

}
