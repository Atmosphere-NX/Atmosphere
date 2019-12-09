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
#include "os_managed_handle.hpp"

namespace ams::os {

    namespace impl {

        class WaitableHolderOfInterruptEvent;

    }

    class InterruptEvent {
        friend class impl::WaitableHolderOfInterruptEvent;
        NON_COPYABLE(InterruptEvent);
        NON_MOVEABLE(InterruptEvent);
        private:
            ManagedHandle handle;
            bool auto_clear;
            bool is_initialized;
        public:
            InterruptEvent() : auto_clear(true), is_initialized(false) { }
            InterruptEvent(u32 interrupt_id, bool autoclear = true);

            Result Initialize(u32 interrupt_id, bool autoclear = true);
            void Finalize();

            void Reset();
            void Wait();
            bool TryWait();
            bool TimedWait(u64 ns);
    };

}
