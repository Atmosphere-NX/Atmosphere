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
#include <stratosphere/os/os_event_types.hpp>
#include <stratosphere/os/os_event_api.hpp>

namespace ams::os {

    class Event {
        NON_COPYABLE(Event);
        NON_MOVEABLE(Event);
        private:
            EventType event;
        public:
            explicit Event(EventClearMode clear_mode) {
                InitializeEvent(std::addressof(this->event), false, clear_mode);
            }

            ~Event() {
                FinalizeEvent(std::addressof(this->event));
            }

            void Wait() {
                return WaitEvent(std::addressof(this->event));
            }

            bool TryWait() {
                return TryWaitEvent(std::addressof(this->event));
            }

            bool TimedWait(TimeSpan timeout) {
                return TimedWaitEvent(std::addressof(this->event), timeout);
            }

            void Signal() {
                return SignalEvent(std::addressof(this->event));
            }

            void Clear() {
                return ClearEvent(std::addressof(this->event));
            }

            operator EventType &() {
                return this->event;
            }

            operator const EventType &() const {
                return this->event;
            }

            EventType *GetBase() {
                return std::addressof(this->event);
            }
    };

}
