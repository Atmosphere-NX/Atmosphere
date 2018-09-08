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
    public:
        HosMutex() {
            mutexInit(&this->m);
        }

        void lock() {
            mutexLock(&this->m);
        }

        void unlock() {
            mutexUnlock(&this->m);
        }

        bool try_lock() {
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
        void lock() {
            rmutexLock(&this->m);
        }
        void unlock() {
            rmutexUnlock(&this->m);
        }
        bool try_lock() {
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
            condvarInit(&cv);
        }

        Result WaitTimeout(u64 timeout) {
            return condvarWaitTimeout(&cv, &m, timeout);
        }

        Result Wait() {
            return condvarWait(&cv, &m);
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
            condvarInit(&cv);
        }

        HosSemaphore(u64 c) : count(c) {
            mutexInit(&m);
            condvarInit(&cv);
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
                condvarWait(&cv, &m);
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
