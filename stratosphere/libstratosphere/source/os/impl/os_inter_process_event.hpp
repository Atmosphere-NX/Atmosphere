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
#include <stratosphere.hpp>

namespace ams::os::impl {

    class WaitableHolderOfInterProcessEvent;

    class InterProcessEvent {
        friend class WaitableHolderOfInterProcessEvent;
        NON_COPYABLE(InterProcessEvent);
        NON_MOVEABLE(InterProcessEvent);
        private:
            Handle read_handle;
            Handle write_handle;
            bool manage_read_handle;
            bool manage_write_handle;
            bool auto_clear;
            bool is_initialized;
        public:
            InterProcessEvent() : is_initialized(false) { /* ... */ }
            InterProcessEvent(bool autoclear);
            ~InterProcessEvent();

            Result Initialize(bool autoclear = true);
            void   Initialize(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, bool autoclear = true);
            Handle DetachReadableHandle();
            Handle DetachWritableHandle();
            Handle GetReadableHandle() const;
            Handle GetWritableHandle() const;
            void Finalize();

            void Signal();
            void Reset();
            void Wait();
            bool TryWait();
            bool TimedWait(u64 ns);
    };

}
