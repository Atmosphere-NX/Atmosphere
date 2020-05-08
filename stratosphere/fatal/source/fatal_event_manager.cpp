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
#include <stratosphere.hpp>
#include "fatal_event_manager.hpp"

namespace ams::fatal::srv {

    FatalEventManager::FatalEventManager() : lock(false) {
        /* Just create all the events. */
        for (size_t i = 0; i < FatalEventManager::NumFatalEvents; i++) {
            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(this->events[i]), os::EventClearMode_AutoClear, true));
        }
    }

    Result FatalEventManager::GetEvent(const os::SystemEventType **out) {
        std::scoped_lock lk{this->lock};

        /* Only allow GetEvent to succeed NumFatalEvents times. */
        R_UNLESS(this->num_events_gotten < FatalEventManager::NumFatalEvents, ResultTooManyEvents());

        *out = std::addressof(this->events[this->num_events_gotten++]);
        return ResultSuccess();
    }

    void FatalEventManager::SignalEvents() {
        for (size_t i = 0; i < FatalEventManager::NumFatalEvents; i++) {
            os::SignalSystemEvent(std::addressof(this->events[i]));
        }
    }

}
