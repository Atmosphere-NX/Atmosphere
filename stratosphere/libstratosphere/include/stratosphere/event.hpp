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
#include <algorithm>
#include <vector>

#include "iwaitable.hpp"
#include "results.hpp"

class IEvent : public IWaitable {
    public:
        /* Information members. */
        Handle r_h;
        Handle w_h;
        bool autoclear;
    public:
        IEvent(bool a = false) : r_h(INVALID_HANDLE), w_h(INVALID_HANDLE), autoclear(a) { }
        IEvent(Handle r, bool a = false) : r_h(r), w_h(INVALID_HANDLE), autoclear(a) { }
        IEvent(Handle r, Handle w, bool a = false) : r_h(r), w_h(w), autoclear(a) { }

        ~IEvent() {
            if (r_h != INVALID_HANDLE) {
                svcCloseHandle(r_h);
            }
            if (w_h != INVALID_HANDLE) {
                svcCloseHandle(w_h);
            }
        }

        /* Make it non-copyable */
        IEvent() = delete;
        IEvent(const IEvent &) = delete;
        IEvent& operator=(const IEvent&) = delete;


        bool IsAutoClear() {
            return this->autoclear;
        }

        void Clear() {
            std::scoped_lock<HosMutex> lock(this->sig_lock);
            this->is_signaled = false;
            if (this->w_h != INVALID_HANDLE) {
                svcClearEvent(this->w_h);
            } else if (this->r_h != INVALID_HANDLE) {
                svcResetSignal(this->r_h);
            }
        }

        void Signal() {
            std::scoped_lock<HosMutex> lock(this->sig_lock);

            if (this->w_h == INVALID_HANDLE && this->r_h != INVALID_HANDLE) {
                /* We can't signal an event if we only have a read handle. */
                std::abort();
            }

            if (this->w_h == INVALID_HANDLE && this->is_signaled) {
                return;
            }

            this->is_signaled = true;

            if (this->w_h != INVALID_HANDLE) {
                svcSignalEvent(this->w_h);
            } else {
                this->NotifyManagerSignaled();
            }
        }

        virtual Result HandleSignaled(u64 timeout) = 0;

        /* IWaitable */
        virtual Handle GetHandle() override {
            return this->r_h;
        }
};

template<class F>
class HosEvent : public IEvent {
    private:
        F callback;
    public:
        HosEvent(F f, bool a = false) : IEvent(a), callback(std::move(f)) { }
        HosEvent(Handle r, F f, bool a = false) : IEvent(r, a), callback(std::move(f)) { }
        HosEvent(Handle r, Handle w, F f, bool a = false) : IEvent(r, w, a), callback(std::move(f)) { }

        virtual Result HandleSignaled(u64 timeout) override {
            if (this->IsAutoClear()) {
                this->Clear();
            }
            return this->callback(timeout);
        }
};

template <class F>
static IEvent *CreateHosEvent(F f, bool autoclear = false) {
    return new HosEvent<F>(INVALID_HANDLE, INVALID_HANDLE, std::move(f), autoclear);
}

template <class F>
static IEvent *CreateSystemEvent(F f, bool autoclear = false) {
    Handle w_h, r_h;
    R_ASSERT(svcCreateEvent(&w_h, &r_h));
    return new HosEvent<F>(r_h, w_h, std::move(f), autoclear);
}

template <class F>
static IEvent *CreateInterruptEvent(F f, u64 irq, bool autoclear = false) {
    Handle r_h;
    /* flag is "rising edge vs level". */
    R_ASSERT(svcCreateInterruptEvent(&r_h, irq, autoclear ? 0 : 1));
    return new HosEvent<F>(r_h, INVALID_HANDLE, std::move(f), autoclear);
}

template <bool a = false>
static IEvent *CreateWriteOnlySystemEvent() {
    return CreateSystemEvent([](u64 timeout) -> Result { std::abort(); }, a);
}

template <class F>
static IEvent *LoadReadOnlySystemEvent(Handle r_h, F f, bool autoclear = false) {
    return new HosEvent<F>(r_h, f, autoclear);
}
