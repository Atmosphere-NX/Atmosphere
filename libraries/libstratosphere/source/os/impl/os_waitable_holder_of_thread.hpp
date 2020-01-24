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
#include "os_waitable_holder_base.hpp"

namespace ams::os::impl {

    /* Nintendo implements this as a user wait object, operating on Thread state. */
    /* Libnx doesn't have an equivalent, so we'll use the thread's handle for kernel semantics. */
    class WaitableHolderOfThread : public WaitableHolderOfKernelObject {
        private:
            Thread *thread;
        public:
            explicit WaitableHolderOfThread(Thread *t) : thread(t) { /* ... */ }

            /* IsSignaled, GetHandle both implemented. */
            virtual TriBool IsSignaled() const override {
                return TriBool::Undefined;
            }

            virtual Handle GetHandle() const override {
                return this->thread->GetHandle();
            }
    };

}
