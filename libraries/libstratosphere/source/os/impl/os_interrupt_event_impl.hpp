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
#include <stratosphere.hpp>

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_interrupt_event_target_impl.os.horizon.hpp"
#else
    #error "Unknown OS for ams::os::InterruptEventImpl"
#endif

namespace ams::os::impl {

    class InterruptEventImpl {
        private:
            InterruptEventTargetImpl impl;
        public:
            explicit InterruptEventImpl(InterruptName name, EventClearMode clear_mode) : impl(name, clear_mode) { /* ... */ }

            void Clear() {
                return this->impl.Clear();
            }

            void Wait() {
                return this->impl.Wait();
            }

            bool TryWait() {
                return this->impl.TryWait();
            }

            bool TimedWait(TimeSpan timeout) {
                return this->impl.TimedWait(timeout);
            }

            TriBool IsSignaled() {
                return this->impl.IsSignaled();
            }

            Handle GetHandle() const {
                return this->impl.GetHandle();
            }
    };

}
