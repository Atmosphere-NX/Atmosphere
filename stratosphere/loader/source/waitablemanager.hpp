#pragma once
#include <switch.h>
#include <vector>

#include "iwaitable.hpp"

class WaitableManager {
    std::vector<IWaitable *> waitables;
    
    u64 timeout;
    
    public:
        WaitableManager(u64 t) : waitables(0), timeout(t) { }
        ~WaitableManager() {
            /* This should call the destructor for every waitable. */
            for (auto & waitable : waitables) {
                delete waitable;
            }
            waitables.clear();
        }
        
        unsigned int get_num_signalable();       
        void add_waitable(IWaitable *waitable);
        void process();
};