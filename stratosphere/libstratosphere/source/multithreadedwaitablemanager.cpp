/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <switch.h>

#include <algorithm>
#include <functional>
#include <mutex>

#include <stratosphere/multithreadedwaitablemanager.hpp>

void MultiThreadedWaitableManager::process() {
    Result rc;
    for (unsigned int i = 0; i < num_threads; i++) {
        if (R_FAILED((rc = threadStart(&threads[i])))) {
            fatalSimple(rc);
        }
    }
    MultiThreadedWaitableManager::thread_func(this);
}

void MultiThreadedWaitableManager::process_until_timeout() {
    /* TODO: Panic. */
}

void MultiThreadedWaitableManager::add_waitable(IWaitable *waitable) {
    std::scoped_lock lk{this->lock};
    this->to_add_waitables.push_back(waitable);
    waitable->set_manager(this);
    this->new_waitable_event->signal_event();
}


IWaitable *MultiThreadedWaitableManager::get_waitable() {
    std::vector<Handle> handles;
    
    int handle_index = 0;
    Result rc;
    std::scoped_lock lk{this->get_waitable_lock};
    while (1) {
        /* Sort waitables by priority. */
        std::sort(this->waitables.begin(), this->waitables.end(), IWaitable::compare);
        
        /* Copy out handles. */
        handles.resize(this->waitables.size());
        std::transform(this->waitables.begin(), this->waitables.end(), handles.begin(), [](IWaitable *w) { return w->get_handle(); });
        
        rc = svcWaitSynchronization(&handle_index, handles.data(), this->waitables.size(), this->timeout);
        IWaitable *w = this->waitables[handle_index];
        if (R_SUCCEEDED(rc)) {
            std::for_each(waitables.begin(), waitables.begin() + handle_index, std::mem_fn(&IWaitable::update_priority));
            this->waitables.erase(this->waitables.begin() + handle_index);
        } else if (rc == 0xEA01) {
            /* Timeout. */
            std::for_each(waitables.begin(), waitables.end(), std::mem_fn(&IWaitable::update_priority));
        } else if (rc != 0xF601 && rc != 0xE401) {
            /* TODO: Panic. When can this happen? */
        } else {
            std::for_each(waitables.begin(), waitables.begin() + handle_index, std::mem_fn(&IWaitable::update_priority));
            this->waitables.erase(this->waitables.begin() + handle_index);
            delete w;
        }
        
        /* Do deferred callback for each waitable. */
        for (auto & waitable : this->waitables) {
            if (waitable->get_deferred()) {
                waitable->handle_deferred();
            }
        }
        
        /* Return waitable. */
        if (R_SUCCEEDED(rc)) {
            if (w == this->new_waitable_event) {
                w->handle_signaled(0);
                this->waitables.push_back(w);
            } else {
                return w;
            }
        }
    }
}

Result MultiThreadedWaitableManager::add_waitable_callback(void *arg, Handle *handles, size_t num_handles, u64 timeout) {
    MultiThreadedWaitableManager *this_ptr = (MultiThreadedWaitableManager *)arg;
    svcClearEvent(handles[0]);
    std::scoped_lock lk{this_ptr->lock};
    this_ptr->waitables.insert(this_ptr->waitables.end(), this_ptr->to_add_waitables.begin(), this_ptr->to_add_waitables.end());
    this_ptr->to_add_waitables.clear();
    return 0;
}

void MultiThreadedWaitableManager::thread_func(void *t) {
    MultiThreadedWaitableManager *this_ptr = (MultiThreadedWaitableManager *)t;
    while (1) {
        IWaitable *w = this_ptr->get_waitable();
        if (w) {
            Result rc = w->handle_signaled(0);
            if (rc == 0xF601) {
                /* Close! */
                delete w;
            } else {
                this_ptr->add_waitable(w);
            }
        }
    }
}
