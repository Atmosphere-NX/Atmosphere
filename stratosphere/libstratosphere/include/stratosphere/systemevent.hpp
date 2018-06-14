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
