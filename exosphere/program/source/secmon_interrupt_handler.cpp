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
#include <exosphere.hpp>
#include "secmon_interrupt_handler.hpp"
#include "secmon_error.hpp"

namespace ams::secmon {

    namespace {

        constexpr inline int InterruptHandlersMax = 4;

        constinit InterruptHandler g_handlers[InterruptHandlersMax] = {};
        constinit int g_interrupt_ids[InterruptHandlersMax]         = {};
        constinit u8 g_interrupt_core_masks[InterruptHandlersMax]   = {};

    }

    void SetInterruptHandler(int interrupt_id, u8 core_mask, InterruptHandler handler) {
        for (int i = 0; i < InterruptHandlersMax; ++i) {
            if (g_interrupt_ids[i] == 0) {
                g_interrupt_ids[i]        = interrupt_id;
                g_handlers[i]             = handler;
                g_interrupt_core_masks[i] = core_mask;
                return;
            }
        }

        AMS_ABORT("Failed to register interrupt handler");
    }

    void HandleInterrupt() {
        /* Get the interrupt id. */
        const int interrupt_id = gic::GetInterruptRequestId();
        if (interrupt_id >= gic::InterruptCount) {
            /* Invalid interrupt number, just return. */
            return;
        }

        /* Check each handler. */
        for (int i = 0; i < InterruptHandlersMax; ++i) {
            if (g_interrupt_ids[i] == interrupt_id) {
                /* Validate that we can invoke the handler. */
                AMS_ABORT_UNLESS((g_interrupt_core_masks[i] & (1u << hw::GetCurrentCoreId())) != 0);

                /* Invoke the handler. */
                g_handlers[i]();
                gic::SetEndOfInterrupt(interrupt_id);
                return;
            }
        }

        AMS_ABORT("Failed to find interrupt handler.");
    }

}
