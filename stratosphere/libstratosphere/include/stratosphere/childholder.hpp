#pragma once
#include <switch.h>
#include <vector>

#include "iwaitable.hpp"

class ChildWaitableHolder : public IWaitable {
    protected:
        std::vector<IWaitable *> children;
    
    public:        
        /* Implicit constructor. */
    
        void add_child(IWaitable *child) {
            this->children.push_back(child);
        }
        
        virtual ~ChildWaitableHolder() {
            for (unsigned int i = 0; i < this->children.size(); i++) {         
                delete this->children[i];
            }
            this->children.clear();
        }
    
        /* IWaitable */
        virtual unsigned int get_num_waitables() {
            unsigned int n = 0;
            for (unsigned int i = 0; i < this->children.size(); i++) {
                if (this->children[i]) {
                    n += this->children[i]->get_num_waitables();
                }
            }
            return n;
        }
        
        virtual void get_waitables(IWaitable **dst) {
            unsigned int n = 0;
            for (unsigned int i = 0; i < this->children.size(); i++) {
                if (this->children[i]) {
                    this->children[i]->get_waitables(&dst[n]);
                    n += this->children[i]->get_num_waitables();
                }
            }
        }
        
        virtual void delete_child(IWaitable *child) {
            unsigned int i;
            for (i = 0; i < this->children.size(); i++) {
                if (this->children[i] == child) {
                    break;
                }
            }
            
            if (i == this->children.size()) {
                /* TODO: Panic, because this isn't our child. */
            } else {
                delete this->children[i];
                this->children.erase(this->children.begin() + i);
            }
        }
        
        virtual Handle get_handle() {
            /* We don't have a handle. */
            return 0;
        }
        
        
        virtual void handle_deferred() {
            /* TODO: Panic, because we can never defer a server. */
        }
        
        virtual Result handle_signaled(u64 timeout) {
            /* TODO: Panic, because we can never be signalled. */
            return 0;
        }
};