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
#include "os_event.hpp"

namespace ams::os {

    class WaitableHolder;

    namespace impl {

        class InterProcessEvent;

    }

    enum class SystemEventState {
        Uninitialized,
        Event,
        InterProcessEvent,
    };

    class SystemEvent {
        friend class WaitableHolder;
        NON_COPYABLE(SystemEvent);
        NON_MOVEABLE(SystemEvent);
        private:
            union {
                util::TypedStorage<Event, sizeof(Event), alignof(Event)> storage_for_event;
                util::TypedStorage<impl::InterProcessEvent, 3 * sizeof(Handle), alignof(Handle)> storage_for_inter_process_event;
            };
            SystemEventState state;
        private:
            Event &GetEvent();
            const Event &GetEvent() const;
            impl::InterProcessEvent &GetInterProcessEvent();
            const impl::InterProcessEvent &GetInterProcessEvent() const;
        public:
            SystemEvent() : state(SystemEventState::Uninitialized) { /* ... */ }
            SystemEvent(bool inter_process, bool autoclear = true);
            SystemEvent(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, bool autoclear = true);
            SystemEvent(Handle read_handle, bool manage_read_handle, bool autoclear = true) : SystemEvent(read_handle, manage_read_handle, INVALID_HANDLE, false, autoclear) { /* ... */ }
            ~SystemEvent();

            Result InitializeAsEvent(bool autoclear = true);
            Result InitializeAsInterProcessEvent(bool autoclear = true);
            void AttachHandles(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, bool autoclear = true);
            void AttachReadableHandle(Handle read_handle, bool manage_read_handle, bool autoclear = true);
            void AttachWritableHandle(Handle write_handle, bool manage_write_handle, bool autoclear = true);
            Handle DetachReadableHandle();
            Handle DetachWritableHandle();
            Handle GetReadableHandle() const;
            Handle GetWritableHandle() const;
            void Finalize();

            SystemEventState GetState() const {
                return this->state;
            }

            void Signal();
            void Reset();
            void Wait();
            bool TryWait();
            bool TimedWait(u64 ns);
    };

}
