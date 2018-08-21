#include <switch.h>

#include <algorithm>
#include <functional>
#include <mutex>

#include <stratosphere/waitablemanager.hpp>

void WaitableManager::add_waitable(IWaitable *waitable) {
    std::scoped_lock lk{this->lock};
    this->to_add_waitables.push_back(waitable);
    waitable->set_manager(this);
    this->has_new_items = true;
}

void WaitableManager::process_internal(bool break_on_timeout) {
    std::vector<Handle> handles;
    
    int handle_index = 0;
    Result rc;
    
    while (1) {
        /* Add new items, if relevant. */
        if (this->has_new_items) {
            std::scoped_lock lk{this->lock};
            this->waitables.insert(this->waitables.end(), this->to_add_waitables.begin(), this->to_add_waitables.end());
            this->to_add_waitables.clear();
            this->has_new_items = false;
        }
                
        /* Sort waitables by priority. */
        std::sort(this->waitables.begin(), this->waitables.end(), IWaitable::compare);
        
        /* Copy out handles. */
        handles.resize(this->waitables.size());
        std::transform(this->waitables.begin(), this->waitables.end(), handles.begin(), [](IWaitable *w) { return w->get_handle(); });
        
        if(this->waitables.size() == 0) {
            continue;
        }
        if(this->waitables.size() > 0x40) {
            panic("PANIC: Too many waitables in waitablemanager");
        }
        rc = svcWaitSynchronization(&handle_index, handles.data(), this->waitables.size(), this->timeout);
        if (R_SUCCEEDED(rc)) {
            /* Handle a signaled waitable. */
            /* TODO: What timeout should be passed here? */
                        
            rc = this->waitables[handle_index]->handle_signaled(0);
            
            std::for_each(waitables.begin(), waitables.begin() + handle_index, std::mem_fn(&IWaitable::update_priority));
        } else if (rc == 0xEA01) {
            /* Timeout. */
            std::for_each(waitables.begin(), waitables.end(), std::mem_fn(&IWaitable::update_priority));
            if (break_on_timeout) {
                return;
            }
        } else if (rc == 0xE401) {
            /* Invalid handle */
            /* handle_index does not get updated when this happens 
            so we can't really do anything */
            panic("PANIC: Invalid handle in waitablemanager");

            /* however switchbrew says that svcWaitSynchronization
            does not accept 0xFFFF8001 or 0xFFFF8000 as handles
            so we could at least remove them if any exists
            
            for(auto it = waitables.begin(); it != waitables.end(); it++) {
                if(*it->get_handle() == 0xFFFF8000 || *it->get_handle() == 0xFFFF8001) {
                    waitables.erase(it);
                }
            }
            */
        } else if (rc != 0xF601) {
            /* TODO: When can this happen? */
            panic("PANIC: unhandled result code in waitablemanager");
        }
        
        if (rc == 0xF601) {
            /* handles[handle_index] was closed! */
            
            /* Close the handle. */
            svcCloseHandle(handles[handle_index]);
            
            IWaitable  *to_delete = this->waitables[handle_index];
            
            /* If relevant, remove from waitables. */
            this->waitables.erase(this->waitables.begin() + handle_index);
            
            /* Delete it. */
            delete to_delete;
                        
            std::for_each(waitables.begin(), waitables.begin() + handle_index, std::mem_fn(&IWaitable::update_priority));
        }
        
        /* Do deferred callback for each waitable. */
        for (auto & waitable : this->waitables) {
            if (waitable->get_deferred()) {
                waitable->handle_deferred();
            }
        }
    }
}

void WaitableManager::process() {
    WaitableManager::process_internal(false);
}

void WaitableManager::process_until_timeout() {
    WaitableManager::process_internal(true);
}
