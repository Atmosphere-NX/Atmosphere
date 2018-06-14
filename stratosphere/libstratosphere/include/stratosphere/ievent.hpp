#pragma once
#include <switch.h>
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
            for (auto &h : this->handles) {
                svcCloseHandle(h);
            }
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
        
        static Result PanicCallback(Handle *handles, size_t num_handles, u64 timeout) {
            /* TODO: Panic. */
            return 0xCAFE;
        } 
};