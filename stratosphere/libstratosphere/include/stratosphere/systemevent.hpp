#pragma once
#include <switch.h>

#include "iwaitable.hpp"
#include "ievent.hpp"

#define SYSTEMEVENT_INDEX_WAITHANDLE 0
#define SYSTEMEVENT_INDEX_SGNLHANDLE 1

class SystemEvent : IEvent {
    public:
        SystemEvent(EventCallback callback) : IEvent(0, callback) {
            Handle wait_h;
            Handle sig_h;
            if (R_FAILED(svcCreateEvent(&wait_h, &sig_h))) {
                /* TODO: Panic. */
            }
            
            this->handles.push_back(wait_h);
            this->handles.push_back(sig_h);
        }
        
        virtual Result signal_event() {
            return svcSignalEvent(this->handles[SYSTEMEVENT_INDEX_SGNLHANDLE]);
        }  
};