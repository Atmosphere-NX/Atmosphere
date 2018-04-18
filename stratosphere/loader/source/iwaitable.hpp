#pragma once

#include <switch.h>

class IWaitable {
    u64 wait_priority = 0;
    IWaitable *parent_waitable;
    public:
        virtual ~IWaitable();
        virtual unsigned int get_num_waitables() = 0;
        virtual void get_waitables(IWaitable **dst) = 0;
        virtual void delete_child(IWaitable *child) = 0;
        virtual Handle get_handle() = 0;
        virtual Result handle_signaled() = 0;
        
        bool has_parent() {
            return this->parent_waitable != NULL;
        }
        
        IWaitable *get_parent() {
            return this->parent_waitable;
        }
        
        void update_priority() {
            static u64 g_cur_priority = 0;
            this->wait_priority = g_cur_priority++;
        }
        
        static bool compare(IWaitable *a, IWaitable *b) {
            return (a->wait_priority < b->wait_priority);
        }
};