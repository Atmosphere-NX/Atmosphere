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
 
#pragma once

#include <switch.h>
#include <memory>

#include "../meta_tools.hpp"

#include "waitable_manager_base.hpp"
#include "event.hpp"
#include "ipc.hpp"
#include "servers.hpp"

#include "scope_guard.hpp"

static inline Handle GetCurrentThreadHandle() {
    return threadGetCurHandle();
}

struct DefaultManagerOptions {
    static constexpr size_t PointerBufferSize = 0;
    static constexpr size_t MaxDomains = 0;
    static constexpr size_t MaxDomainObjects = 0;
};

struct DomainEntry {
    ServiceObjectHolder obj_holder;
    IDomainObject *owner = nullptr;
};

template<typename ManagerOptions = DefaultManagerOptions>
class WaitableManager : public SessionManagerBase {
    private:
        /* Domain Manager */
        HosMutex domain_lock;
        std::array<std::weak_ptr<IDomainObject>, ManagerOptions::MaxDomains> domains;
        std::array<bool, ManagerOptions::MaxDomains> is_domain_allocated;
        std::array<DomainEntry, ManagerOptions::MaxDomainObjects> domain_objects;
        
        /* Waitable Manager */
        std::vector<IWaitable *> to_add_waitables;
        std::vector<IWaitable *> waitables;
        u32 num_threads;
        Thread *threads;
        HosMutex process_lock;
        HosMutex signal_lock;
        HosMutex add_lock;
        bool has_new_waitables = false;
                
        IWaitable *next_signaled = nullptr;
        Handle cur_thread_handle = INVALID_HANDLE;
    public:
        WaitableManager(u32 n, u32 ss = 0x8000) : num_threads(n-1) { 
            u32 prio;
            u32 cpuid = svcGetCurrentProcessorNumber();
            Result rc;
            threads = new Thread[num_threads];
            if (num_threads) {
                if (R_FAILED((rc = svcGetThreadPriority(&prio, CUR_THREAD_HANDLE)))) {
                    fatalSimple(rc);
                }
                for (unsigned int i = 0; i < num_threads; i++) {
                    threads[i] = {0};
                    threadCreate(&threads[i], &WaitableManager::ProcessLoop, this, ss, prio, cpuid);
                }
            }
        }
        
        ~WaitableManager() override {
            /* This should call the destructor for every waitable. */
            std::for_each(waitables.begin(), waitables.end(), std::default_delete<IWaitable>{});
            
            /* TODO: Exit the threads? */
        }

        virtual void AddWaitable(IWaitable *w) override {
            std::scoped_lock lk{this->add_lock};
            this->to_add_waitables.push_back(w);
            w->SetManager(this);
            this->has_new_waitables = true;
            this->CancelSynchronization();
        }

        virtual void CancelSynchronization() {
            svcCancelSynchronization(this->cur_thread_handle);
        }
        
        virtual void NotifySignaled(IWaitable *w) override {
            std::scoped_lock lk{this->signal_lock};
            if (this->next_signaled == nullptr) {
                this->next_signaled = w;
            }
            this->CancelSynchronization();
        }
        
        virtual void Process() override {
            /* Add initial set of waitables. */
            AddWaitablesInternal();
            
            Result rc;
            for (unsigned int i = 0; i < num_threads; i++) {
                if (R_FAILED((rc = threadStart(&threads[i])))) {
                    fatalSimple(rc);
                }
            }
                                    
            ProcessLoop(this);
        }
    private:
        static void ProcessLoop(void *t) {
            WaitableManager *this_ptr = (WaitableManager *)t;
            while (true) {
                IWaitable *w = this_ptr->GetWaitable();
                if (w) {
                    Result rc = w->HandleSignaled(0);
                    if (rc == 0xF601) {
                        /* Close! */
                        delete w;
                    } else {
                        this_ptr->AddWaitable(w);
                    }
                }
                
            }
        }
        
        IWaitable *GetWaitable() {
            std::scoped_lock lk{this->process_lock};
            this->next_signaled = nullptr;
            IWaitable *result = nullptr;
            
            /* Add new waitables, if any. */
            AddWaitablesInternal();
            
            this->cur_thread_handle = GetCurrentThreadHandle();
            ON_SCOPE_EXIT {
                this->cur_thread_handle = INVALID_HANDLE;
            };
            
            /* First, see if anything's already signaled. */
            for (auto &w : this->waitables) {
                if (w->IsSignaled()) {
                    result = w;
                }
            }
            
            /* It's possible somebody signaled us while we were iterating. */
            {
                std::scoped_lock lk{this->signal_lock};
                if (this->next_signaled != nullptr) result = this->next_signaled;
            }
                        
            if (result == nullptr) {
                std::vector<Handle> handles;
                std::vector<IWaitable *> wait_list;
                
                int handle_index = 0;
                Result rc;
                while (result == nullptr) {
                    /* Sort waitables by priority. */
                    std::sort(this->waitables.begin(), this->waitables.end(), IWaitable::Compare);
                    
                    /* Copy out handles. */
                    handles.resize(this->waitables.size());
                    wait_list.resize(this->waitables.size());
                    unsigned int num_handles = 0;
                    for (unsigned int i = 0; i < this->waitables.size(); i++) {
                        Handle h = this->waitables[i]->GetHandle();
                        if (h != INVALID_HANDLE) {
                            wait_list[num_handles] = this->waitables[i];
                            handles[num_handles++] = h;
                        }
                    }
                    
      
                    /* Do deferred callback for each waitable. This has to happen before we wait on anything else. */
                    for (auto & waitable : this->waitables) {
                        if (waitable->IsDeferred()) {
                            waitable->HandleDeferred();
                        }
                    }
                    
                    /* Wait forever. */
                    rc = svcWaitSynchronization(&handle_index, handles.data(), num_handles, U64_MAX);
                    
                    IWaitable *w = wait_list[handle_index];
                    size_t w_ind = std::distance(this->waitables.begin(), std::find(this->waitables.begin(), this->waitables.end(), w));
                    
                    if (R_SUCCEEDED(rc)) {
                        std::for_each(waitables.begin(), waitables.begin() + w_ind, std::mem_fn(&IWaitable::UpdatePriority));
                        result = w;
                    } else if (rc == 0xEA01) {
                        /* Timeout: Just update priorities. */
                        std::for_each(waitables.begin(), waitables.end(), std::mem_fn(&IWaitable::UpdatePriority));
                    } else if (rc == 0xEC01) {
                        /* svcCancelSynchronization was called. */
                        AddWaitablesInternal();
                        {
                            std::scoped_lock lk{this->signal_lock};
                            if (this->next_signaled != nullptr) {
                                result = this->next_signaled;
                            }
                        }
                    } else if (rc != 0xF601 && rc != 0xE401) {
                        std::abort();
                    } else {  
                        this->waitables.erase(this->waitables.begin() + w_ind);
                        std::for_each(waitables.begin(), waitables.begin() + w_ind - 1, std::mem_fn(&IWaitable::UpdatePriority));
                        delete w;
                    }
                }
            }
            
            this->waitables.erase(std::remove_if(this->waitables.begin(), this->waitables.end(), [&](IWaitable *w) { return w == result; }), this->waitables.end());
            
            return result;
        }
        
        void AddWaitablesInternal() {
            std::scoped_lock lk{this->add_lock};
            if (this->has_new_waitables) {
                this->waitables.insert(this->waitables.end(), this->to_add_waitables.begin(), this->to_add_waitables.end());
                this->to_add_waitables.clear();
                this->has_new_waitables = false;
            }
        }
    /* Session Manager */
    public:
        virtual void AddSession(Handle server_h, ServiceObjectHolder &&service) override {
            this->AddWaitable(new ServiceSession(server_h, ManagerOptions::PointerBufferSize, std::move(service)));
        }
    
    /* Domain Manager */
    public:
        virtual std::shared_ptr<IDomainObject> AllocateDomain() override {
            std::scoped_lock lk{this->domain_lock};
            for (size_t i = 0; i < ManagerOptions::MaxDomains; i++) {
                if (!this->is_domain_allocated[i]) {
                    auto new_domain = std::make_shared<IDomainObject>(this);
                    this->domains[i] = new_domain;
                    this->is_domain_allocated[i] = true;
                    return new_domain;
                }
            }
            return nullptr;
        }
        
        void FreeDomain(IDomainObject *domain) override {
            std::scoped_lock lk{this->domain_lock};
            for (size_t i = 0; i < ManagerOptions::MaxDomainObjects; i++) {
                FreeObject(domain, i+1);
            }
            for (size_t i = 0; i < ManagerOptions::MaxDomains; i++) {
                auto observe = this->domains[i].lock();
                if (observe.get() == domain) {
                    this->is_domain_allocated[i] = false;
                    break;
                }
            }
        }
        
        virtual Result ReserveObject(IDomainObject *domain, u32 *out_object_id) override {
            std::scoped_lock lk{this->domain_lock};
            for (size_t i = 0; i < ManagerOptions::MaxDomainObjects; i++) {
                if (this->domain_objects[i].owner == nullptr) {
                    this->domain_objects[i].owner = domain;
                    *out_object_id = i+1;
                    return 0;
                }
            }
            return 0x25A0A;
        }
        
        virtual Result ReserveSpecificObject(IDomainObject *domain, u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (this->domain_objects[object_id-1].owner == nullptr) {
                this->domain_objects[object_id-1].owner = domain;
                return 0;
            }
            return 0x25A0A;
        }
        
        virtual void SetObject(IDomainObject *domain, u32 object_id, ServiceObjectHolder&& holder) override {
            std::scoped_lock lk{this->domain_lock};
            if (this->domain_objects[object_id-1].owner == domain) {
                this->domain_objects[object_id-1].obj_holder = std::move(holder);
            }
        }
        
        virtual ServiceObjectHolder *GetObject(IDomainObject *domain, u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (this->domain_objects[object_id-1].owner == domain) {
                return &this->domain_objects[object_id-1].obj_holder;
            }
            return nullptr;
        }
        
        virtual Result FreeObject(IDomainObject *domain, u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (this->domain_objects[object_id-1].owner == domain) {
                this->domain_objects[object_id-1].obj_holder.Reset();
                this->domain_objects[object_id-1].owner = nullptr;
                return 0x0;
            }
            return 0x3D80B;
        }
        
        virtual Result ForceFreeObject(u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (this->domain_objects[object_id-1].owner != nullptr) {
                this->domain_objects[object_id-1].obj_holder.Reset();
                this->domain_objects[object_id-1].owner = nullptr;
                return 0x0;
            }
            return 0x3D80B;
        }
};

