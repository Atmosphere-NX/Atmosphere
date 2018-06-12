#pragma once

#include <switch.h>

#include "waitablemanagerbase.hpp"

class WaitableManager;

class IWaitable {
    private:
        u64 wait_priority = 0;
        bool is_deferred = false;
        WaitableManagerBase *manager;
    public:
        virtual ~IWaitable() { }
        
        virtual void handle_deferred() = 0;
        virtual Handle get_handle() = 0;
        virtual Result handle_signaled(u64 timeout) = 0;
                
        WaitableManager *get_manager() {
            return (WaitableManager *)this->manager;
        }
        
        void set_manager(WaitableManagerBase *m) {
            this->manager = m;
        }
        
        void update_priority() {
            if (manager) {
                this->wait_priority = this->manager->get_priority();
            }
        }
        
        bool get_deferred() {
            return this->is_deferred;
        }
        
        void set_deferred(bool d) {
            this->is_deferred = d;
        }
        
        static bool compare(IWaitable *a, IWaitable *b) {
            return (a->wait_priority < b->wait_priority) && !a->is_deferred;
        }
};

#include "waitablemanager.hpp"