/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include "fatal_types.hpp"
#include "fatal_event_manager.hpp"

static FatalEventManager g_event_manager;

FatalEventManager *GetEventManager() {
    return &g_event_manager;
}

FatalEventManager::FatalEventManager() {
    /* Just create all the events. */
    for (size_t i = 0; i < FatalEventManager::NumFatalEvents; i++) {
        if (R_FAILED(eventCreate(&this->events[i], true))) {
            std::abort();
        }
    }
}

Result FatalEventManager::GetEvent(Handle *out) {
    std::scoped_lock<HosMutex> lk{this->lock};
    
    /* Only allow GetEvent to succeed NumFatalEvents times. */
    if (this->events_gotten >= FatalEventManager::NumFatalEvents) {
        return FatalResult_TooManyEvents;
    }
    
    *out = this->events[this->events_gotten++].revent;
    return 0;
}

void FatalEventManager::SignalEvents() {
    for (size_t i = 0; i < FatalEventManager::NumFatalEvents; i++) {
        eventFire(&this->events[i]);
    }
}
