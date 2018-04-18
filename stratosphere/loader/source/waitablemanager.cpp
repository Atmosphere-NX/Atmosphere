#include <switch.h>

#include <algorithm>
#include <cstdio>

#include "waitablemanager.hpp"


unsigned int WaitableManager::get_num_signalable() {
    unsigned int n = 0;
    for (auto & waitable : this->waitables) {
        n += waitable->get_num_waitables();
    }
    return n;
}

void WaitableManager::add_waitable(IWaitable *waitable) {
    this->waitables.push_back(waitable);
}

void WaitableManager::process() {
    std::vector<IWaitable *> signalables;
    std::vector<Handle> handles;
    
    int handle_index = 0;
    Result rc;
    
    while (1) {
        /* Create vector of signalable items. */
        signalables.resize(this->get_num_signalable(), NULL);
        unsigned int n = 0;
        for (auto & waitable : this->waitables) {
            waitable->get_waitables(signalables.data() + n);
            n += waitable->get_num_waitables();
        }
        
        /* Sort signalables by priority. */
        std::sort(signalables.begin(), signalables.end(), IWaitable::compare);
        
        /* Copy out handles. */
        handles.resize(signalables.size());
        std::transform(signalables.begin(), signalables.end(), handles.begin(), [](IWaitable *w) { return w->get_handle(); });
        
        rc = svcWaitSynchronization(&handle_index, handles.data(), handles.size(), this->timeout);
        if (R_SUCCEEDED(rc)) {
            /* Handle a signaled waitable. */
            /* TODO: What timeout should be passed here? */
                        
            rc = signalables[handle_index]->handle_signaled(0);
            
            for (int i = 0; i < handle_index; i++) {
                signalables[i]->update_priority();
            }
        } else if (rc == 0xEA01) {
            /* Timeout. */
            for (auto & waitable : signalables) {
                waitable->update_priority();
            }
        } else if (rc != 0xF601) {
            /* TODO: Panic. When can this happen? */
        }
        
        if (rc == 0xF601) {
            /* handles[handle_index] was closed! */
            
            /* Close the handle. */
            svcCloseHandle(handles[handle_index]);
            
            /* If relevant, remove from waitables. */
            this->waitables.erase(std::remove(this->waitables.begin(), this->waitables.end(), signalables[handle_index]), this->waitables.end());
            
            /* Delete it. */
            if (signalables[handle_index]->has_parent()) {
                signalables[handle_index]->get_parent()->delete_child(signalables[handle_index]);
            } else {
                delete signalables[handle_index];
            }
            
            for (int i = 0; i < handle_index; i++) {
                signalables[i]->update_priority();
            }
        } 
    }
}