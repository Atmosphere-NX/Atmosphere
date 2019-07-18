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
#include <switch/arm/counter.h>
#include <mutex>
#include "results.hpp"
#include "auto_handle.hpp"

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

        void Lock() {
            lock();
        }

        void Unlock() {
            unlock();
        }

        bool TryLock() {
            return try_lock();
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

        void Lock() {
            lock();
        }

        void Unlock() {
            unlock();
        }

        bool TryLock() {
            return try_lock();
        }
};

class HosCondVar {
    private:
        CondVar cv;
    public:
        HosCondVar() {
            condvarInit(&cv);
        }

        Result TimedWait(u64 timeout, HosMutex *hm) {
            return TimedWait(timeout, hm->GetMutex());
        }

        Result Wait(HosMutex *hm) {
            return Wait(hm->GetMutex());
        }

        Result TimedWait(u64 timeout, Mutex *m) {
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

class TimeoutHelper {
    private:
        u64 end_tick;
    public:
        TimeoutHelper(u64 ns) {
            /* Special case zero-time timeouts. */
            if (ns == 0) {
                end_tick = 0;
                return;
            }

            u64 cur_tick = armGetSystemTick();
            this->end_tick = cur_tick + NsToTick(ns) + 1;
        }

        static inline u64 NsToTick(u64 ns) {
            return (ns * 12) / 625;
        }

        static inline u64 TickToNs(u64 tick) {
            return (tick * 625) / 12;
        }

        u64 NsUntilTimeout() {
            u64 diff = TickToNs(this->end_tick - armGetSystemTick());

            if (TimedOut()) {
                return 0;
            }

            return diff;
        }

        bool TimedOut() {
            if (this->end_tick == 0) {
                return true;
            }

            return armGetSystemTick() >= this->end_tick;
        }
};

class HosSignal {
    private:
        CondVar cv;
        Mutex m;
        bool signaled;
    public:
        HosSignal() {
            condvarInit(&cv);
            mutexInit(&m);
            signaled = false;
        }

        void Signal() {
            mutexLock(&m);
            signaled = true;
            condvarWakeAll(&cv);
            mutexUnlock(&m);
        }

        void Reset() {
            mutexLock(&m);
            signaled = false;
            mutexUnlock(&m);
        }

        void Wait() {
            mutexLock(&m);

            while (!signaled) {
                condvarWait(&cv, &m);
            }

            mutexUnlock(&m);
        }

        bool TryWait() {
            mutexLock(&m);
            bool success = signaled;
            mutexUnlock(&m);
            return success;
        }

        Result TimedWait(u64 ns) {
            mutexLock(&m);
            TimeoutHelper timeout_helper(ns);

            while (!signaled) {
                if (R_FAILED(condvarWaitTimeout(&cv, &m, timeout_helper.NsUntilTimeout()))) {
                    return false;
                }
            }

            mutexUnlock(&m);
            return true;
        }
};

class HosThread {
    private:
        Thread thr = {};
    public:
        HosThread() {}

        Result Initialize(ThreadFunc entry, void *arg, size_t stack_sz, int prio, int cpuid = -2) {
            return threadCreate(&this->thr, entry, arg, stack_sz, prio, cpuid);
        }

        Handle GetHandle() const {
            return this->thr.handle;
        }

        Result Start() {
            return threadStart(&this->thr);
        }

        Result Join() {
            R_TRY(threadWaitForExit(&this->thr));
            R_TRY(threadClose(&this->thr));
            return ResultSuccess;
        }

        Result CancelSynchronization() {
            return svcCancelSynchronization(this->thr.handle);
        }
};