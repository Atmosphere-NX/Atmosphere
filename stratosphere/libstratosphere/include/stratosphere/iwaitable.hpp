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

#include "waitable_manager_base.hpp"
#include "hossynch.hpp"

class IWaitable {
    private:
        u64 wait_priority = 0;
        bool is_deferred = false;
        WaitableManagerBase *manager = nullptr;
    protected:
        HosMutex sig_lock;
        bool is_signaled = false;
    public:
        virtual ~IWaitable() = default;

        virtual Result HandleDeferred() {
            /* By default, HandleDeferred panics, because object shouldn't be deferrable. */
            std::abort();
        }

        bool IsSignaled() {
            std::scoped_lock<HosMutex> lock(this->sig_lock);
            return this->is_signaled;
        }

        virtual Handle GetHandle() = 0;
        virtual Result HandleSignaled(u64 timeout) = 0;

        WaitableManagerBase *GetManager() {
            return this->manager;
        }

        void SetManager(WaitableManagerBase *m) {
            this->manager = m;
        }

        void UpdatePriority() {
            if (manager) {
                this->wait_priority = this->manager->GetNextPriority();
            }
        }

        bool IsDeferred() {
            return this->is_deferred;
        }

        void SetDeferred(bool d) {
            this->is_deferred = d;
        }

        static bool Compare(IWaitable *a, IWaitable *b) {
            return (a->wait_priority < b->wait_priority) && !a->IsDeferred() && (a->GetHandle() != INVALID_HANDLE);
        }

        void NotifyManagerSignaled() {
            if (this->manager) {
                this->manager->NotifySignaled(this);
            }
        }
};
