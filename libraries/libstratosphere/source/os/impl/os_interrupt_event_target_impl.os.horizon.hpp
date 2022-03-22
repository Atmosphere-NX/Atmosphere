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

namespace ams::os::impl {

    class InterruptEventHorizonImpl {
        private:
            svc::Handle m_handle;
            bool m_manual_clear;
        public:
            explicit InterruptEventHorizonImpl(InterruptName name, EventClearMode mode);
            ~InterruptEventHorizonImpl();

            void Clear();
            void Wait();
            bool TryWait();
            bool TimedWait(TimeSpan timeout);

            TriBool IsSignaled() {
                return TriBool::Undefined;
            }

            NativeHandle GetHandle() const {
                return m_handle;
            }
    };

    using InterruptEventTargetImpl = InterruptEventHorizonImpl;

}
