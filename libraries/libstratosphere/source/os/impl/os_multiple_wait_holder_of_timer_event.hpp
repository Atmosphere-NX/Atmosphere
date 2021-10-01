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
#include <stratosphere.hpp>
#include "os_timer_event_helper.hpp"
#include "os_multiple_wait_holder_base.hpp"
#include "os_multiple_wait_object_list.hpp"

namespace ams::os::impl {

    class MultiWaitHolderOfTimerEvent : public MultiWaitHolderOfUserObject {
        private:
            TimerEventType *event;
        private:
            TriBool IsSignaledImpl() const {
                TimeSpan cur_time = this->GetMultiWait()->GetCurrentTime();
                UpdateSignalStateAndRecalculateNextTimeToWakeupUnsafe(this->event, cur_time);
                return this->event->signaled ? TriBool::True : TriBool::False;
            }
        public:
            explicit MultiWaitHolderOfTimerEvent(TimerEventType *e) : event(e) { /* ... */ }

            /* IsSignaled, Link, Unlink implemented. */
            virtual TriBool IsSignaled() const override {
                std::scoped_lock lk(GetReference(this->event->cs_timer_event));
                return this->IsSignaledImpl();
            }

            virtual TriBool LinkToObjectList() override {
                std::scoped_lock lk(GetReference(this->event->cs_timer_event));

                GetReference(this->event->multi_wait_object_list_storage).LinkMultiWaitHolder(*this);
                return this->IsSignaledImpl();
            }

            virtual void UnlinkFromObjectList() override {
                std::scoped_lock lk(GetReference(this->event->cs_timer_event));

                GetReference(this->event->multi_wait_object_list_storage).UnlinkMultiWaitHolder(*this);
            }

            /* Gets the amount of time remaining until this wakes up. */
            virtual TimeSpan GetAbsoluteWakeupTime() const override {
                std::scoped_lock lk(GetReference(this->event->cs_timer_event));

                if (this->event->timer_state == TimerEventType::TimerState_Stop) {
                    return TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max());
                }

                return GetReference(this->event->next_time_to_wakeup);
            }
    };

}
