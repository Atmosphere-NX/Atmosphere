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

        constexpr bool IsValidInterruptType(ams::svc::InterruptType type) {
            switch (type) {
                case ams::svc::InterruptType_Edge:
                case ams::svc::InterruptType_Level:
                    return true;
                default:
                    return false;
            }
        }

        Result CreateInterruptEvent(ams::svc::Handle *out, int32_t interrupt_id, ams::svc::InterruptType type) {
            /* Validate the type. */
            R_UNLESS(IsValidInterruptType(type), svc::ResultInvalidEnumValue());

            /* Check whether the interrupt is allowed. */
            auto &process = GetCurrentProcess();
            R_UNLESS(process.IsPermittedInterrupt(interrupt_id), svc::ResultNotFound());

            /* Get the current handle table. */
            auto &handle_table = process.GetHandleTable();

            /* Create the interrupt event. */
            KInterruptEvent *event = KInterruptEvent::Create();
            R_UNLESS(event != nullptr, svc::ResultOutOfResource());
            ON_SCOPE_EXIT { event->Close(); };

            /* Initialize the event. */
            R_TRY(event->Initialize(interrupt_id, type));

            /* Register the event. */
            KInterruptEvent::Register(event);

            /* Add the event to the handle table. */
            R_TRY(handle_table.Add(out, event));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result CreateInterruptEvent64(ams::svc::Handle *out_read_handle, int32_t interrupt_id, ams::svc::InterruptType interrupt_type) {
        return CreateInterruptEvent(out_read_handle, interrupt_id, interrupt_type);
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateInterruptEvent64From32(ams::svc::Handle *out_read_handle, int32_t interrupt_id, ams::svc::InterruptType interrupt_type) {
        return CreateInterruptEvent(out_read_handle, interrupt_id, interrupt_type);
    }

}
