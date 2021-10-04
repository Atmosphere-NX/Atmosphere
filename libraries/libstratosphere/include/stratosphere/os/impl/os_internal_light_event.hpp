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

#pragma once
#include <vapours.hpp>

#if defined(ATMOSPHERE_OS_HORIZON)
    #include <stratosphere/os/impl/os_internal_light_event_impl.os.horizon.hpp>
#else
    #error "Unknown OS for ams::os::impl::InternalLightEventImpl"
#endif

namespace ams::os::impl {

    class InternalLightEvent {
        private:
            InternalLightEventImpl m_impl;
        public:
            explicit InternalLightEvent(bool signaled) : m_impl(signaled) { /* ... */ }

            ALWAYS_INLINE void SignalWithAutoClear() { return m_impl.SignalWithAutoClear(); }
            ALWAYS_INLINE void SignalWithManualClear() { return m_impl.SignalWithManualClear(); }

            ALWAYS_INLINE void Clear() { return m_impl.Clear(); }

            ALWAYS_INLINE void WaitWithAutoClear() { return m_impl.WaitWithAutoClear(); }
            ALWAYS_INLINE void WaitWithManualClear() { return m_impl.WaitWithManualClear(); }

            ALWAYS_INLINE bool TryWaitWithAutoClear() { return m_impl.TryWaitWithAutoClear(); }
            ALWAYS_INLINE bool TryWaitWithManualClear() { return m_impl.TryWaitWithManualClear(); }

            ALWAYS_INLINE bool TimedWaitWithAutoClear(const TimeoutHelper &timeout_helper) { return m_impl.TimedWaitWithAutoClear(timeout_helper); }
            ALWAYS_INLINE bool TimedWaitWithManualClear(const TimeoutHelper &timeout_helper) { return m_impl.TimedWaitWithManualClear(timeout_helper); }
    };

    using InternalLightEventStorage = util::TypedStorage<InternalLightEvent>;

}
