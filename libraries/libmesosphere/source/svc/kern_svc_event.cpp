/*
 * Copyright (c) Atmosphère-NX
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

        Result SignalEvent(ams::svc::Handle event_handle) {
            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Get the writable event. */
            KScopedAutoObject event = handle_table.GetObject<KEvent>(event_handle);
            R_UNLESS(event.IsNotNull(), svc::ResultInvalidHandle());

            return event->Signal();
        }

        Result ClearEvent(ams::svc::Handle event_handle) {
            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Try to clear the writable event. */
            {
                KScopedAutoObject event = handle_table.GetObject<KEvent>(event_handle);
                if (event.IsNotNull()) {
                    return event->Clear();
                }
            }

            /* Try to clear the readable event. */
            {
                KScopedAutoObject readable_event = handle_table.GetObject<KReadableEvent>(event_handle);
                if (readable_event.IsNotNull()) {
                    if (auto * const interrupt_event = readable_event->DynamicCast<KInterruptEvent *>(); interrupt_event != nullptr) {
                        return interrupt_event->Clear();
                    } else {
                        return readable_event->Clear();
                    }
                }
            }

            return svc::ResultInvalidHandle();
        }

        Result CreateEvent(ams::svc::Handle *out_write, ams::svc::Handle *out_read) {
            /* Get the current process and handle table. */
            auto &process      = GetCurrentProcess();
            auto &handle_table = process.GetHandleTable();

            /* Declare the event we're going to allocate. */
            KEvent *event;

            /* Reserve a new event from the process resource limit. */
            KScopedResourceReservation event_reservation(std::addressof(process), ams::svc::LimitableResource_EventCountMax);
            if (event_reservation.Succeeded()) {
                /* Allocate an event normally. */
                event = KEvent::Create();
            } else {
                /* We couldn't reserve an event. Check that we support dynamically expanding the resource limit. */
                R_UNLESS(process.GetResourceLimit() == std::addressof(Kernel::GetSystemResourceLimit()), svc::ResultLimitReached());
                R_UNLESS(KTargetSystem::IsDynamicResourceLimitsEnabled(),                                svc::ResultLimitReached());

                /* Try to allocate an event from unused slab memory. */
                event = KEvent::CreateFromUnusedSlabMemory();
                R_UNLESS(event != nullptr, svc::ResultLimitReached());

                /* We successfully allocated an event, so add the object we allocated to the resource limit. */
                Kernel::GetSystemResourceLimit().Add(ams::svc::LimitableResource_EventCountMax, 1);
            }

            /* Check that we successfully created an event. */
            R_UNLESS(event != nullptr, svc::ResultOutOfResource());

            /* Initialize the event. */
            event->Initialize();

            /* Commit the event reservation. */
            event_reservation.Commit();

            /* Ensure that we clean up the event (and its only references are handle table) on function end. */
            ON_SCOPE_EXIT {
                event->GetReadableEvent().Close();
                event->Close();
            };

            /* Register the event. */
            KEvent::Register(event);

            /* Add the event to the handle table. */
            R_TRY(handle_table.Add(out_write, event));

            /* Ensure that we maintaing a clean handle state on exit. */
            auto handle_guard = SCOPE_GUARD { handle_table.Remove(*out_write); };

            /* Add the readable event to the handle table. */
            R_TRY(handle_table.Add(out_read, std::addressof(event->GetReadableEvent())));

            /* We succeeded! */
            handle_guard.Cancel();
            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SignalEvent64(ams::svc::Handle event_handle) {
        return SignalEvent(event_handle);
    }

    Result ClearEvent64(ams::svc::Handle event_handle) {
        return ClearEvent(event_handle);
    }

    Result CreateEvent64(ams::svc::Handle *out_write_handle, ams::svc::Handle *out_read_handle) {
        return CreateEvent(out_write_handle, out_read_handle);
    }

    /* ============================= 64From32 ABI ============================= */

    Result SignalEvent64From32(ams::svc::Handle event_handle) {
        return SignalEvent(event_handle);
    }

    Result ClearEvent64From32(ams::svc::Handle event_handle) {
        return ClearEvent(event_handle);
    }

    Result CreateEvent64From32(ams::svc::Handle *out_write_handle, ams::svc::Handle *out_read_handle) {
        return CreateEvent(out_write_handle, out_read_handle);
    }

}
