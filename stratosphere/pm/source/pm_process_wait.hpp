#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class ProcessWaiter final : public IWaitable {
    public:
        Registration::Process process;
        
        ProcessWaiter(Registration::Process p) : process(p) {
            
        }
        
        ProcessWaiter(Registration::Process *p) {
            this->process = *p;
        }
        
        Registration::Process *get_process() { 
            return &this->process; 
        }
        
        /* IWaitable */        
        Handle get_handle() override {
            return this->process.handle;
        }
        
        void handle_deferred() override {
            /* TODO: Panic, because we can never be deferred. */
        }
        
        Result handle_signaled(u64 timeout) override {
            Registration::HandleSignaledProcess(this->get_process());
            return 0;
        }
};

class ProcessList final : public IWaitable {
    private:      
        HosRecursiveMutex mutex;
    public:
        std::vector<ProcessWaiter *> process_waiters;
        
        void Lock() {
            this->mutex.Lock();
        }
        
        void Unlock() {
            this->mutex.Unlock();
        }
        
        bool TryLock() {
            return this->mutex.TryLock();
        }
        
        /* IWaitable */
        
        Handle get_handle() override {
            /* TODO: Panic, because we don't have a handle. */
            return 0;
        }
        
        void handle_deferred() override {
            /* TODO: Panic, because we can never be deferred. */
        }
        
        Result handle_signaled(u64 timeout) override {
            /* TODO: Panic, because we can never be signaled. */         
            return 0;
        }
};

