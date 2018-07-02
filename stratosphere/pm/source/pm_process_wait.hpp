#pragma once
#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>
#include "pm_registration.hpp"

class ProcessWaiter final : public IWaitable {
    public:
        std::shared_ptr<Registration::Process> process;
        
        ProcessWaiter(std::shared_ptr<Registration::Process> p) : process(p) {
            /* ... */
        }
        
        std::shared_ptr<Registration::Process> get_process() { 
            return this->process; 
        }
        
        /* IWaitable */        
        Handle get_handle() override {
            return this->process->handle;
        }
        
        void handle_deferred() override {
            /* TODO: Panic, because we can never be deferred. */
        }
        
        Result handle_signaled(u64 timeout) override {
            return Registration::HandleSignaledProcess(this->get_process());
        }
};

class ProcessList final {
    private:      
        HosRecursiveMutex mutex;
        WaitableManager *manager;
    public:
        std::vector<std::shared_ptr<Registration::Process>> processes;
        
        auto get_unique_lock() {
            return std::unique_lock{this->mutex};
        }

        void set_manager(WaitableManager *manager) {
            this->manager = manager;
        }
        
        WaitableManager *get_manager() {
            return this->manager;
        }
};

