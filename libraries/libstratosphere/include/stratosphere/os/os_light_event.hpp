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
#include <stratosphere/os/os_event_common.hpp>
#include <stratosphere/os/os_light_event_types.hpp>
#include <stratosphere/os/os_light_event_api.hpp>

namespace ams::os {

    class LightEvent {
        NON_COPYABLE(LightEvent);
        NON_MOVEABLE(LightEvent);
        private:
            LightEventType m_event;
        public:
            explicit LightEvent(EventClearMode clear_mode) {
                InitializeLightEvent(std::addressof(m_event), false, clear_mode);
            }

            ~LightEvent() {
                FinalizeLightEvent(std::addressof(m_event));
            }

            void Wait() {
                return WaitLightEvent(std::addressof(m_event));
            }

            bool TryWait() {
                return TryWaitLightEvent(std::addressof(m_event));
            }

            bool TimedWait(TimeSpan timeout) {
                return TimedWaitLightEvent(std::addressof(m_event), timeout);
            }

            void Signal() {
                return SignalLightEvent(std::addressof(m_event));
            }

            void Clear() {
                return ClearLightEvent(std::addressof(m_event));
            }

            operator LightEventType &() {
                return m_event;
            }

            operator const LightEventType &() const {
                return m_event;
            }

            LightEventType *GetBase() {
                return std::addressof(m_event);
            }
    };

}
