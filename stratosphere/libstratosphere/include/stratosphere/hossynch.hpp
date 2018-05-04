#pragma once
#include <switch.h>

class HosMutex {
    private:
        Mutex m;
    public:
        HosMutex() {
            mutexInit(&this->m);
        }
        
        void Lock() {
            mutexLock(&this->m);
        }
        
        void Unlock() {
            mutexUnlock(&this->m);
        }
        
        bool TryLock() {
            return mutexTryLock(&this->m);
        }
};

class HosRecursiveMutex {
    private:
        RMutex m;
    public:
        HosRecursiveMutex() {
            rmutexInit(&this->m);
        }
        
        void Lock() {
            rmutexLock(&this->m);
        }
        
        void Unlock() {
            rmutexUnlock(&this->m);
        }
        
        bool TryLock() {
            return rmutexTryLock(&this->m);
        }
};

class HosCondVar {
    private:
        CondVar cv;
        Mutex m;
    public:
        HosCondVar() {
            mutexInit(&m);
            condvarInit(&cv, &m);
        }
        
        Result WaitTimeout(u64 timeout) {
            return condvarWaitTimeout(&cv, timeout);
        }
        
        Result Wait() {
            return condvarWait(&cv);
        }
        
        Result Wake(int num) {
            return condvarWake(&cv, num);
        }
        
        Result WakeOne() {
            return condvarWakeOne(&cv);
        }
        
        Result WakeAll() {
            return condvarWakeAll(&cv);
        }
};

class HosSemaphore {
    private:
        CondVar cv;
        Mutex m;
        u64 count;
    public:
        HosSemaphore() {
            count = 0;
            mutexInit(&m);
            condvarInit(&cv, &m);
        }
        
        HosSemaphore(u64 c) : count(c) {
            mutexInit(&m);
            condvarInit(&cv, &m);
        }
        
        void Signal() {
            mutexLock(&this->m);
            count++;
            condvarWakeOne(&cv);
            mutexUnlock(&this->m);
        }
        
        void Wait() {
            mutexLock(&this->m);
            while (!count) {
                condvarWait(&cv);
            }
            count--;
            mutexUnlock(&this->m);
        }
        
        bool TryWait() {
            mutexLock(&this->m);
            bool success = false;
            if (count) {
                count--;
                success = true;
            }
            mutexUnlock(&this->m);
            return success;
        }
};
