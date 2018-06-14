#pragma once
#include <switch.h>
#include <vector>

#include "waitablemanagerbase.hpp"
#include "iwaitable.hpp"
#include "hossynch.hpp"

class IWaitable;

class WaitableManager : public WaitableManagerBase {
    protected:
        std::vector<IWaitable *> to_add_waitables;
        std::vector<IWaitable *> waitables;
        u64 timeout;
        HosMutex lock;
        std::atomic_bool has_new_items;
    private:
        void process_internal(bool break_on_timeout);
    public:
        WaitableManager(u64 t) : waitables(0), timeout(t), has_new_items(false) { }
        ~WaitableManager() override {
            /* This should call the destructor for every waitable. */
            for (auto & waitable : waitables) {
                delete waitable;
            }
            waitables.clear();
        }
        
        virtual void add_waitable(IWaitable *waitable);
        virtual void process();
        virtual void process_until_timeout();
};