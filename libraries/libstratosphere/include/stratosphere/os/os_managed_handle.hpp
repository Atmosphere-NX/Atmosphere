/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "os_common_types.hpp"

namespace ams::os {

    class ManagedHandle {
        NON_COPYABLE(ManagedHandle);
        private:
            Handle hnd;
        public:
            constexpr ManagedHandle() : hnd(INVALID_HANDLE) { /* ... */ }
            constexpr ManagedHandle(Handle h) : hnd(h) { /* ... */ }
            ~ManagedHandle() {
                if (this->hnd != INVALID_HANDLE) {
                    R_ABORT_UNLESS(svcCloseHandle(this->hnd));
                    this->hnd = INVALID_HANDLE;
                }
            }

            ManagedHandle(ManagedHandle&& rhs) {
                this->hnd = rhs.hnd;
                rhs.hnd = INVALID_HANDLE;
            }

            ManagedHandle &operator=(ManagedHandle&& rhs) {
                rhs.Swap(*this);
                return *this;
            }

            explicit operator bool() const {
                return this->hnd != INVALID_HANDLE;
            }

            void Swap(ManagedHandle &rhs) {
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

            void Detach() {
                const Handle h = this->Move();
                AMS_UNUSED(h);
            }

            void Reset(Handle h) {
                ManagedHandle(h).Swap(*this);
            }

            void Clear() {
                this->Reset(INVALID_HANDLE);
            }
    };

}
