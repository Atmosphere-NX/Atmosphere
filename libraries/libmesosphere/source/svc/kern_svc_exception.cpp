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

        void Break(ams::svc::BreakReason break_reason, uintptr_t address, size_t size) {
            /* Log for debug that Break was called. */
            MESOSPHERE_LOG("%s: Break(%08x, %016lx, %zu)\n", GetCurrentProcess().GetName(), static_cast<u32>(break_reason), address, size);

            /* If the current process is attached to debugger, notify it. */
            if (GetCurrentProcess().IsAttachedToDebugger()) {
                MESOSPHERE_UNIMPLEMENTED();
            }

            /* If the break is only a notification, we're done. */
            if ((break_reason & ams::svc::BreakReason_NotificationOnlyFlag) != 0) {
                return;
            }

            /* TODO */
            if (size == sizeof(u32)) {
                MESOSPHERE_LOG("DEBUG: %08x\n", *reinterpret_cast<u32 *>(address));
            }
            MESOSPHERE_PANIC("Break was called\n");
        }

    }

    /* =============================    64 ABI    ============================= */

    void Break64(ams::svc::BreakReason break_reason, ams::svc::Address arg, ams::svc::Size size) {
        return Break(break_reason, arg, size);
    }

    void ReturnFromException64(ams::Result result) {
        MESOSPHERE_PANIC("Stubbed SvcReturnFromException64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    void Break64From32(ams::svc::BreakReason break_reason, ams::svc::Address arg, ams::svc::Size size) {
        return Break(break_reason, arg, size);
    }

    void ReturnFromException64From32(ams::Result result) {
        MESOSPHERE_PANIC("Stubbed SvcReturnFromException64From32 was called.");
    }

}
