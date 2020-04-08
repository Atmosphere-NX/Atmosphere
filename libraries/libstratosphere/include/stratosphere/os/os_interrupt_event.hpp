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
            InterruptEventType event;
        public:
            explicit InterruptEvent(InterruptName name, EventClearMode clear_mode) {
                InitializeInterruptEvent(std::addressof(this->event), name, clear_mode);
            }

            ~InterruptEvent() {
                FinalizeInterruptEvent(std::addressof(this->event));
            }

            void Wait() {
                return WaitInterruptEvent(std::addressof(this->event));
            }

            bool TryWait() {
                return TryWaitInterruptEvent(std::addressof(this->event));
            }

            bool TimedWait(TimeSpan timeout) {
                return TimedWaitInterruptEvent(std::addressof(this->event), timeout);
            }

            void Clear() {
                return ClearInterruptEvent(std::addressof(this->event));
            }

            operator InterruptEventType &() {
                return this->event;
            }

            operator const InterruptEventType &() const {
                return this->event;
            }

            InterruptEventType *GetBase() {
                return std::addressof(this->event);
            }
    };


}
