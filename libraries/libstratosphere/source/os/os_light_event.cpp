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
#include <stratosphere.hpp>
#include "impl/os_timeout_helper.hpp"

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "impl/os_internal_light_event_impl.os.horizon.hpp"
#else
    #error "Unknown OS for ams::os::impl::InternalLightEventImpl"
#endif

namespace ams::os {
        bool is_auto_clear;
        bool is_initialized;

        impl::InternalLightEventStorage storage;

    void InitializeLightEvent(LightEventType *event, bool signaled, EventClearMode clear_mode) {
        /* Set member variables. */
        event->is_auto_clear  = clear_mode == EventClearMode_AutoClear;
        event->is_initialized = true;

        /* Create object. */
        util::ConstructAt(event->storage, signaled);
    }

    void FinalizeLightEvent(LightEventType *event) {
        /* Set not-initialized. */
        event->is_initialized = false;

        /* Destroy object. */
        util::DestroyAt(event->storage);
    }

    void SignalLightEvent(LightEventType *event) {
        /* Check pre-conditions. */
        AMS_ASSERT(event->is_initialized);

        /* Signal. */
        if (event->is_auto_clear) {
            return util::GetReference(event->storage).SignalWithAutoClear();
        } else {
            return util::GetReference(event->storage).SignalWithManualClear();
        }
    }

    void WaitLightEvent(LightEventType *event) {
        /* Check pre-conditions. */
        AMS_ASSERT(event->is_initialized);

        /* Wait. */
        if (event->is_auto_clear) {
            return util::GetReference(event->storage).WaitWithAutoClear();
        } else {
            return util::GetReference(event->storage).WaitWithManualClear();
        }
    }

    bool TryWaitLightEvent(LightEventType *event) {
        /* Check pre-conditions. */
        AMS_ASSERT(event->is_initialized);

        /* Wait. */
        if (event->is_auto_clear) {
            return util::GetReference(event->storage).TryWaitWithAutoClear();
        } else {
            return util::GetReference(event->storage).TryWaitWithManualClear();
        }
    }

    bool TimedWaitLightEvent(LightEventType *event, TimeSpan timeout) {
        /* Check pre-conditions. */
        AMS_ASSERT(event->is_initialized);
        AMS_ASSERT(timeout.GetNanoSeconds() >= 0);

        /* Create timeout helper. */
        impl::TimeoutHelper timeout_helper(timeout);

        /* Wait. */
        if (event->is_auto_clear) {
            return util::GetReference(event->storage).TimedWaitWithAutoClear(timeout_helper);
        } else {
            return util::GetReference(event->storage).TimedWaitWithManualClear(timeout_helper);
        }
    }

    void ClearLightEvent(LightEventType *event) {
        /* Check pre-conditions. */
        AMS_ASSERT(event->is_initialized);

        return util::GetReference(event->storage).Clear();
    }

}
