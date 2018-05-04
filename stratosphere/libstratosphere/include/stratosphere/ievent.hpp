#pragma once
#include <switch.h>
#include <vector>

#include "iwaitable.hpp"

typedef Result (*EventCallback)(Handle *handles, size_t num_handles, u64 timeout);

class IEvent : public IWaitable {
    protected:
        std::vector<Handle> handles;
        EventCallback callback;
    
    public:
        IEvent(Handle wait_h, EventCallback callback) {
            if (wait_h) {
                this->handles.push_back(wait_h);
            }
            this->callback = callback;
        }
        
        ~IEvent() {
            for (auto &h : this->handles) {
                svcCloseHandle(h);
            }
        }
        
        virtual Result signal_event() = 0;
        
        /* IWaitable */
        virtual unsigned int get_num_waitables() {
            if (handles.size() > 0) {
                return 1;
            }
            return 0;
        }
        
        virtual void get_waitables(IWaitable **dst) {
            if (handles.size() > 0) {
                dst[0] = this;
            }
        }
        
        virtual void delete_child(IWaitable *child) {
            /* TODO: Panic, an event can never be a parent. */
        }
        
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
            return this->callback(this->handles.data(), this->handles.size(), timeout);
        }
        
        static Result PanicCallback(Handle *handles, size_t num_handles, u64 timeout) {
            /* TODO: Panic. */
            return 0xCAFE;
        } 
};