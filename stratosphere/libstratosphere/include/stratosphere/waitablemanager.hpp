#pragma once
#include <switch.h>
#include <algorithm>
#include <memory>
#include <vector>

#include "waitablemanagerbase.hpp"
#include "iwaitable.hpp"
#include "hossynch.hpp"

class IWaitable;

class WaitableManager : public WaitableManagerBase {
    protected:
        std::vector<IWaitable *> to_add_waitables;
        std::vector<IWaitable *> waitables;
        u64 timeout = 0;
        HosMutex lock;
        std::atomic_bool has_new_items = false;
    private:
        void process_internal(bool break_on_timeout);
    public:
        WaitableManager(u64 t) : timeout(t) { }
        ~WaitableManager() override {
            /* This should call the destructor for every waitable. */
            std::for_each(waitables.begin(), waitables.end(), std::default_delete<IWaitable>{});
        }
        
        virtual void add_waitable(IWaitable *waitable);
        virtual void process();
        virtual void process_until_timeout();
};
