/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <functional>

#include "results.hpp"
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
        std::array<uintptr_t, ManagerOptions::MaxDomains> domain_keys;
        std::array<bool, ManagerOptions::MaxDomains> is_domain_allocated;
        std::array<DomainEntry, ManagerOptions::MaxDomainObjects> domain_objects;

        /* Waitable Manager */
        std::vector<IWaitable *> to_add_waitables;
        std::vector<IWaitable *> waitables;
        std::vector<IWaitable *> deferred_waitables;

        u32 num_extra_threads = 0;
        HosThread *threads = nullptr;

        HosMutex process_lock;
        HosMutex signal_lock;
        HosMutex add_lock;
        HosMutex cur_thread_lock;
        HosMutex deferred_lock;
        bool has_new_waitables = false;
        std::atomic<bool> should_stop = false;

        IWaitable *next_signaled = nullptr;
        Handle main_thread_handle = INVALID_HANDLE;
        Handle cur_thread_handle = INVALID_HANDLE;
    public:
        WaitableManager(u32 n, u32 ss = 0x8000) : num_extra_threads(n-1) {
            u32 prio;
            if (num_extra_threads) {
                threads = new HosThread[num_extra_threads];
                R_ASSERT(svcGetThreadPriority(&prio, CUR_THREAD_HANDLE));
                for (unsigned int i = 0; i < num_extra_threads; i++) {
                    R_ASSERT(threads[i].Initialize(&WaitableManager::ProcessLoop, this, ss, prio));
                }
            }
        }

        ~WaitableManager() override {
            /* This should call the destructor for every waitable. */
            std::for_each(to_add_waitables.begin(), to_add_waitables.end(), std::default_delete<IWaitable>{});
            std::for_each(waitables.begin(), waitables.end(), std::default_delete<IWaitable>{});
            std::for_each(deferred_waitables.begin(), deferred_waitables.end(), std::default_delete<IWaitable>{});

            /* If we've reached here, we should already have exited the threads. */
        }

        virtual void AddWaitable(IWaitable *w) override {
            std::scoped_lock lk{this->add_lock};
            this->to_add_waitables.push_back(w);
            w->SetManager(this);
            this->has_new_waitables = true;
            this->CancelSynchronization();
        }

        virtual void RequestStop() {
            this->should_stop = true;
            this->CancelSynchronization();
        }

        virtual void CancelSynchronization() {
            svcCancelSynchronization(GetProcessingThreadHandle());
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

            /* Set main thread handle. */
            this->main_thread_handle = GetCurrentThreadHandle();

            for (unsigned int i = 0; i < num_extra_threads; i++) {
                R_ASSERT(threads[i].Start());
            }

            ProcessLoop(this);
        }
    private:
        void SetProcessingThreadHandle(Handle h) {
            std::scoped_lock<HosMutex> lk{this->cur_thread_lock};
            this->cur_thread_handle = h;
        }

        Handle GetProcessingThreadHandle() {
            std::scoped_lock<HosMutex> lk{this->cur_thread_lock};
            return this->cur_thread_handle;
        }

        static void ProcessLoop(void *t) {
            WaitableManager *this_ptr = (WaitableManager *)t;
            while (true) {
                IWaitable *w = this_ptr->GetWaitable();
                if (this_ptr->should_stop) {
                    if (GetCurrentThreadHandle() == this_ptr->main_thread_handle) {
                        /* Join all threads but the main one. */
                        for (unsigned int i = 0; i < this_ptr->num_extra_threads; i++) {
                            this_ptr->threads[i].Join();
                        }
                        break;
                    } else {
                        /* Return, this will cause thread to exit. */
                        return;
                    }
                }
                if (w) {
                    if (w->HandleSignaled(0) == ResultKernelConnectionClosed) {
                        /* Close! */
                        delete w;
                    } else {
                        if (w->IsDeferred()) {
                            std::scoped_lock lk{this_ptr->deferred_lock};
                            this_ptr->deferred_waitables.push_back(w);
                        } else {
                            this_ptr->AddWaitable(w);
                        }
                    }
                }

                /* We finished processing, and maybe that means we can stop deferring an object. */
                {
                    std::scoped_lock lk{this_ptr->deferred_lock};
                    bool undeferred_any = true;
                    while (undeferred_any) {
                        undeferred_any = false;
                        for (auto it = this_ptr->deferred_waitables.begin(); it != this_ptr->deferred_waitables.end();) {
                            auto w = *it;
                            const bool closed = (w->HandleDeferred() == ResultKernelConnectionClosed);
                            if (closed || !w->IsDeferred()) {
                                /* Remove from the deferred list, set iterator. */
                                it = this_ptr->deferred_waitables.erase(it);
                                if (closed) {
                                    /* Delete the closed waitable. */
                                    delete w;
                                } else {
                                    /* Add to the waitables list. */
                                    this_ptr->AddWaitable(w);
                                    undeferred_any = true;
                                }
                            } else {
                                /* Move on to the next deferred waitable. */
                                it++;
                            }
                        }
                    }
                }
            }
        }

        IWaitable *GetWaitable() {
            std::scoped_lock lk{this->process_lock};

            /* Set processing thread handle while in scope. */
            SetProcessingThreadHandle(GetCurrentThreadHandle());
            ON_SCOPE_EXIT {
                SetProcessingThreadHandle(INVALID_HANDLE);
            };

            /* Prepare variables for result. */
            this->next_signaled = nullptr;
            IWaitable *result = nullptr;

            if (this->should_stop) {
                return nullptr;
            }

            /* Add new waitables, if any. */
            AddWaitablesInternal();

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
                while (result == nullptr) {
                    /* Sort waitables by priority. */
                    std::sort(this->waitables.begin(), this->waitables.end(), IWaitable::Compare);

                    /* Copy out handles. */
                    handles.resize(this->waitables.size());
                    wait_list.resize(this->waitables.size());
                    unsigned int num_handles = 0;

                    /* Try to add waitables to wait list. */
                    for (unsigned int i = 0; i < this->waitables.size(); i++) {
                        Handle h = this->waitables[i]->GetHandle();
                        if (h != INVALID_HANDLE) {
                            wait_list[num_handles] = this->waitables[i];
                            handles[num_handles++] = h;
                        }
                    }

                    /* Wait forever. */
                    const Result wait_res = svcWaitSynchronization(&handle_index, handles.data(), num_handles, U64_MAX);

                    if (this->should_stop) {
                        return nullptr;
                    }

                    if (R_SUCCEEDED(wait_res)) {
                        IWaitable *w = wait_list[handle_index];
                        size_t w_ind = std::distance(this->waitables.begin(), std::find(this->waitables.begin(), this->waitables.end(), w));
                        std::for_each(waitables.begin(), waitables.begin() + w_ind + 1, std::mem_fn(&IWaitable::UpdatePriority));
                        result = w;
                    } else if (wait_res == ResultKernelTimedOut) {
                        /* Timeout: Just update priorities. */
                        std::for_each(waitables.begin(), waitables.end(), std::mem_fn(&IWaitable::UpdatePriority));
                    } else if (wait_res == ResultKernelCancelled) {
                        /* svcCancelSynchronization was called. */
                        AddWaitablesInternal();
                        {
                            std::scoped_lock lk{this->signal_lock};
                            if (this->next_signaled != nullptr) {
                                result = this->next_signaled;
                            }
                        }
                    } else {
                        /* TODO: Consider the following cases that this covers: */
                        /* 7601: Thread termination requested. */
                        /* E401: Handle is dead. */
                        /* E601: Handle list address invalid. */
                        /* EE01: Too many handles. */
                        std::abort();
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
                    this->domain_keys[i] = reinterpret_cast<uintptr_t>(new_domain.get());
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
                if (this->domain_keys[i] == reinterpret_cast<uintptr_t>(domain)) {
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
                    return ResultSuccess;
                }
            }
            return ResultServiceFrameworkOutOfDomainEntries;
        }

        virtual Result ReserveSpecificObject(IDomainObject *domain, u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (object_id > ManagerOptions::MaxDomainObjects || object_id < MinimumDomainId) {
                return ResultServiceFrameworkOutOfDomainEntries;
            }
            if (this->domain_objects[object_id-1].owner == nullptr) {
                this->domain_objects[object_id-1].owner = domain;
                return ResultSuccess;
            }
            return ResultServiceFrameworkOutOfDomainEntries;
        }

        virtual void SetObject(IDomainObject *domain, u32 object_id, ServiceObjectHolder&& holder) override {
            std::scoped_lock lk{this->domain_lock};
            if (object_id > ManagerOptions::MaxDomainObjects || object_id < MinimumDomainId) {
                return;
            }
            if (this->domain_objects[object_id-1].owner == domain) {
                this->domain_objects[object_id-1].obj_holder = std::move(holder);
            }
        }

        virtual ServiceObjectHolder *GetObject(IDomainObject *domain, u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (object_id > ManagerOptions::MaxDomainObjects || object_id < MinimumDomainId) {
                return nullptr;
            }
            if (this->domain_objects[object_id-1].owner == domain) {
                return &this->domain_objects[object_id-1].obj_holder;
            }
            return nullptr;
        }

        virtual Result FreeObject(IDomainObject *domain, u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (object_id > ManagerOptions::MaxDomainObjects || object_id < MinimumDomainId) {
                return ResultHipcDomainObjectNotFound;
            }
            if (this->domain_objects[object_id-1].owner == domain) {
                this->domain_objects[object_id-1].obj_holder.Reset();
                this->domain_objects[object_id-1].owner = nullptr;
                return ResultSuccess;
            }
            return ResultHipcDomainObjectNotFound;
        }

        virtual Result ForceFreeObject(u32 object_id) override {
            std::scoped_lock lk{this->domain_lock};
            if (object_id > ManagerOptions::MaxDomainObjects || object_id < MinimumDomainId) {
                return ResultHipcDomainObjectNotFound;
            }
            if (this->domain_objects[object_id-1].owner != nullptr) {
                this->domain_objects[object_id-1].obj_holder.Reset();
                this->domain_objects[object_id-1].owner = nullptr;
                return ResultSuccess;
            }
            return ResultHipcDomainObjectNotFound;
        }
};

