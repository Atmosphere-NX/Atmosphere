/*
 * Copyright (c) Atmosphère-NX
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
            InterruptEventTargetImpl m_impl;
        public:
            explicit InterruptEventImpl(InterruptName name, EventClearMode clear_mode) : m_impl(name, clear_mode) { /* ... */ }

            void Clear() {
                return m_impl.Clear();
            }

            void Wait() {
                return m_impl.Wait();
            }

            bool TryWait() {
                return m_impl.TryWait();
            }

            bool TimedWait(TimeSpan timeout) {
                return m_impl.TimedWait(timeout);
            }

            TriBool IsSignaled() {
                return m_impl.IsSignaled();
            }

            NativeHandle GetHandle() const {
                return m_impl.GetHandle();
            }
    };

}
