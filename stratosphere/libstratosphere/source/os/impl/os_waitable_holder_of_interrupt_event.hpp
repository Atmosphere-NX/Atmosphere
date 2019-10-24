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
#include "os_waitable_holder_base.hpp"

namespace ams::os::impl {

    class WaitableHolderOfInterruptEvent : public WaitableHolderOfKernelObject {
        private:
            InterruptEvent *event;
        public:
            explicit WaitableHolderOfInterruptEvent(InterruptEvent *e) : event(e) { /* ... */ }

            /* IsSignaled, GetHandle both implemented. */
            virtual TriBool IsSignaled() const override {
                return TriBool::Undefined;
            }

            virtual Handle GetHandle() const override {
                AMS_ASSERT(this->event->is_initialized);
                return this->event->handle.Get();
            }
    };

}
