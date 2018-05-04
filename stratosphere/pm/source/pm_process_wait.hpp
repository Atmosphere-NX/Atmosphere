#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class ProcessWaiter : public IWaitable {
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
        virtual unsigned int get_num_waitables() {
            return 1;
        }
        
        virtual void get_waitables(IWaitable **dst) {
            dst[0] = this;
        }
        
        virtual void delete_child(IWaitable *child) {
            /* TODO: Panic, because we can never have any children. */
        }
        
        virtual Handle get_handle() {
            return this->process.handle;
        }
        
        virtual void handle_deferred() {
            /* TODO: Panic, because we can never be deferred. */
        }
        
        virtual Result handle_signaled(u64 timeout) {
            Registration::HandleSignaledProcess(this->get_process());
            return 0;
        }
};

class ProcessList : public IWaitable {
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
        virtual unsigned int get_num_waitables() {
            return process_waiters.size();
        }
        
        virtual void get_waitables(IWaitable **dst) {
            Lock();
            for (unsigned int i = 0; i < process_waiters.size(); i++) {
                dst[i] = process_waiters[i];
            }
            Unlock();
        }
        
        virtual void delete_child(IWaitable *child) {
            /* TODO: Panic, because we should never be asked to delete a child. */
        }
        
        virtual Handle get_handle() {
            /* TODO: Panic, because we don't have a handle. */
            return 0;
        }
        
        virtual void handle_deferred() {
            /* TODO: Panic, because we can never be deferred. */
        }
        
        virtual Result handle_signaled(u64 timeout) {
            /* TODO: Panic, because we can never be signaled. */         
            return 0;
        }
};

