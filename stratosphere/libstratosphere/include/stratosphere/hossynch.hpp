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

class HosMutex {
    private:
        Mutex m;
        Mutex *GetMutex() {
            return &this->m;
        }
    public:
        HosMutex() {
            mutexInit(GetMutex());
        }
        
        void lock() {
            mutexLock(GetMutex());
        }
        
        void unlock() {
            mutexUnlock(GetMutex());
        }
        
        bool try_lock() {
            return mutexTryLock(GetMutex());
        }
        
    friend class HosCondVar;
};

class HosRecursiveMutex {
    private:
        RMutex m;
        RMutex *GetMutex() {
            return &this->m;
        }
    public:
        HosRecursiveMutex() {
            rmutexInit(GetMutex());
        }
        
        void lock() {
            rmutexLock(GetMutex());
        }
        
        void unlock() {
            rmutexUnlock(GetMutex());
        }
        
        bool try_lock() {
            return rmutexTryLock(GetMutex());
        }
};

class HosCondVar {
    private:
        CondVar cv;
    public:
        HosCondVar() {
            condvarInit(&cv);
        }
        
        Result WaitTimeout(u64 timeout, HosMutex *hm) {
            return WaitTimeout(timeout, hm->GetMutex());
        }
        
        Result Wait(HosMutex *hm) {
            return Wait(hm->GetMutex());
        }
        
        Result WaitTimeout(u64 timeout, Mutex *m) {
            return condvarWaitTimeout(&cv, m, timeout);
        }
        
        Result Wait(Mutex *m) {
            return condvarWait(&cv, m);
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
        Semaphore s;
    public:
        HosSemaphore() {
            semaphoreInit(&s, 0);
        }
        
        HosSemaphore(u64 c) {
            semaphoreInit(&s, c);
        }
        
        void Signal() {
            semaphoreSignal(&s);
        }
        
        void Wait() {
            semaphoreWait(&s);
        }
        
        bool TryWait() {
            return semaphoreTryWait(&s);
        }
};
