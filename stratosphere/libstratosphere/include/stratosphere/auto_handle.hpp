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
#include "defines.hpp"

class AutoHandle {
    NON_COPYABLE(AutoHandle);
    private:
        Handle hnd;
    public:
        AutoHandle() : hnd(INVALID_HANDLE) { /* ... */ }
        AutoHandle(Handle h) : hnd(h) { /* ... */ }
        ~AutoHandle() {
            if (this->hnd != INVALID_HANDLE) {
                svcCloseHandle(this->hnd);
                this->hnd = INVALID_HANDLE;
            }
        }

        AutoHandle(AutoHandle&& rhs) {
            this->hnd = rhs.hnd;
            rhs.hnd = INVALID_HANDLE;
        }

        AutoHandle& operator=(AutoHandle&& rhs) {
            rhs.Swap(*this);
            return *this;
        }

        explicit operator bool() const {
            return this->hnd != INVALID_HANDLE;
        }

        void Swap(AutoHandle& rhs) {
            std::swap(this->hnd, rhs.hnd);
        }

        Handle Get() const {
            return this->hnd;
        }

        Handle *GetPointer() {
            return &this->hnd;
        }

        Handle *GetPointerAndClear() {
            this->Clear();
            return this->GetPointer();
        }

        Handle Move() {
            const Handle h = this->hnd;
            this->hnd = INVALID_HANDLE;
            return h;
        }

        void Reset(Handle h) {
            AutoHandle(h).Swap(*this);
        }

        void Clear() {
            this->Reset(INVALID_HANDLE);
        }
};
