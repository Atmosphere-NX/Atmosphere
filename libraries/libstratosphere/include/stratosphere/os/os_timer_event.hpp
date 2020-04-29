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
#include <stratosphere/os/os_timer_event_types.hpp>
#include <stratosphere/os/os_timer_event_api.hpp>

namespace ams::os {

    class TimerEvent {
        NON_COPYABLE(TimerEvent);
        NON_MOVEABLE(TimerEvent);
        private:
            TimerEventType event;
        public:
            explicit TimerEvent(EventClearMode clear_mode) {
                InitializeTimerEvent(std::addressof(this->event), clear_mode);
            }

            ~TimerEvent() {
                FinalizeTimerEvent(std::addressof(this->event));
            }

            void StartOneShot(TimeSpan first_time) {
                return StartOneShotTimerEvent(std::addressof(this->event), first_time);
            }

            void StartPeriodic(TimeSpan first_time, TimeSpan interval) {
                return StartPeriodicTimerEvent(std::addressof(this->event), first_time, interval);
            }

            void Stop() {
                return StopTimerEvent(std::addressof(this->event));
            }

            void Wait() {
                return WaitTimerEvent(std::addressof(this->event));
            }

            bool TryWait() {
                return TryWaitTimerEvent(std::addressof(this->event));
            }

            void Signal() {
                return SignalTimerEvent(std::addressof(this->event));
            }

            void Clear() {
                return ClearTimerEvent(std::addressof(this->event));
            }

            operator TimerEventType &() {
                return this->event;
            }

            operator const TimerEventType &() const {
                return this->event;
            }

            TimerEventType *GetBase() {
                return std::addressof(this->event);
            }
    };

}
