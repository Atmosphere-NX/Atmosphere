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
#include <stratosphere/os/os_interrupt_event_common.hpp>
#include <stratosphere/os/os_interrupt_event_types.hpp>
#include <stratosphere/os/os_interrupt_event_api.hpp>

namespace ams::os {

    class InterruptEvent {
        NON_COPYABLE(InterruptEvent);
        NON_MOVEABLE(InterruptEvent);
        private:
            InterruptEventType m_event;
        public:
            explicit InterruptEvent(InterruptName name, EventClearMode clear_mode) {
                InitializeInterruptEvent(std::addressof(m_event), name, clear_mode);
            }

            ~InterruptEvent() {
                FinalizeInterruptEvent(std::addressof(m_event));
            }

            void Wait() {
                return WaitInterruptEvent(std::addressof(m_event));
            }

            bool TryWait() {
                return TryWaitInterruptEvent(std::addressof(m_event));
            }

            bool TimedWait(TimeSpan timeout) {
                return TimedWaitInterruptEvent(std::addressof(m_event), timeout);
            }

            void Clear() {
                return ClearInterruptEvent(std::addressof(m_event));
            }

            operator InterruptEventType &() {
                return m_event;
            }

            operator const InterruptEventType &() const {
                return m_event;
            }

            InterruptEventType *GetBase() {
                return std::addressof(m_event);
            }
    };


}
