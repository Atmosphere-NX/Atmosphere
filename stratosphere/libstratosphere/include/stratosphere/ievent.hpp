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
#include <algorithm>
#include <vector>

#include "iwaitable.hpp"

typedef Result (*EventCallback)(void *arg, Handle *handles, size_t num_handles, u64 timeout);

class IEvent : public IWaitable {
    protected:
        std::vector<Handle> handles;
        EventCallback callback;
        void *arg;
    
    public:
        IEvent(Handle wait_h, void *a, EventCallback callback) {
            if (wait_h) {
                this->handles.push_back(wait_h);
            }
            this->arg = a;
            this->callback = callback;
        }
        
        ~IEvent() {
            std::for_each(handles.begin(), handles.end(), svcCloseHandle);
        }
        
        virtual Result signal_event() = 0;
        
        /* IWaitable */                
        virtual Handle get_handle() {
            if (handles.size() > 0) {
                return this->handles[0];
            }
            return 0;
        }
        
        
        virtual void handle_deferred() {
            /* TODO: Panic, because we can never defer an event. */
        }
        
        virtual Result handle_signaled(u64 timeout) {
            return this->callback(this->arg, this->handles.data(), this->handles.size(), timeout);
        }
        
        static Result PanicCallback(void *arg, Handle *handles, size_t num_handles, u64 timeout) {
            /* TODO: Panic. */
            return 0xCAFE;
        } 
};
