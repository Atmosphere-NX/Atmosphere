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
#include <stratosphere/os/os_timer_event_types.hpp>
#include <stratosphere/os/os_timer_event_api.hpp>

namespace ams::os {

    class TimerEvent {
        NON_COPYABLE(TimerEvent);
        NON_MOVEABLE(TimerEvent);
        private:
            TimerEventType m_event;
        public:
            explicit TimerEvent(EventClearMode clear_mode) {
                InitializeTimerEvent(std::addressof(m_event), clear_mode);
            }

            ~TimerEvent() {
                FinalizeTimerEvent(std::addressof(m_event));
            }

            void StartOneShot(TimeSpan first_time) {
                return StartOneShotTimerEvent(std::addressof(m_event), first_time);
            }

            void StartPeriodic(TimeSpan first_time, TimeSpan interval) {
                return StartPeriodicTimerEvent(std::addressof(m_event), first_time, interval);
            }

            void Stop() {
                return StopTimerEvent(std::addressof(m_event));
            }

            void Wait() {
                return WaitTimerEvent(std::addressof(m_event));
            }

            bool TryWait() {
                return TryWaitTimerEvent(std::addressof(m_event));
            }

            void Signal() {
                return SignalTimerEvent(std::addressof(m_event));
            }

            void Clear() {
                return ClearTimerEvent(std::addressof(m_event));
            }

            operator TimerEventType &() {
                return m_event;
            }

            operator const TimerEventType &() const {
                return m_event;
            }

            TimerEventType *GetBase() {
                return std::addressof(m_event);
            }
    };

}
