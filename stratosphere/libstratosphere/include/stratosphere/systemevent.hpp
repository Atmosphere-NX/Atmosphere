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
 
#pragma once
#include <switch.h>

#include "iwaitable.hpp"
#include "ievent.hpp"

#define SYSTEMEVENT_INDEX_WAITHANDLE 0
#define SYSTEMEVENT_INDEX_SGNLHANDLE 1

class SystemEvent final : public IEvent {
    public:
        SystemEvent(void *a, EventCallback callback) : IEvent(0, a, callback) {
            Handle wait_h;
            Handle sig_h;
            if (R_FAILED(svcCreateEvent(&sig_h, &wait_h))) {
                /* TODO: Panic. */
            }
            
            this->handles.push_back(wait_h);
            this->handles.push_back(sig_h);
        }
        
        Result signal_event() override {
            return svcSignalEvent(this->handles[SYSTEMEVENT_INDEX_SGNLHANDLE]);
        }  
};
