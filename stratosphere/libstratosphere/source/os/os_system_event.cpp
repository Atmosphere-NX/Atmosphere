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
#include <stratosphere.hpp>
#include "impl/os_waitable_holder_impl.hpp"

namespace ams::os {

    SystemEvent::SystemEvent(bool inter_process, bool autoclear) : state(SystemEventState::Uninitialized) {
        if (inter_process) {
            R_ASSERT(this->InitializeAsInterProcessEvent(autoclear));
        } else {
            R_ASSERT(this->InitializeAsEvent(autoclear));
        }
    }

    SystemEvent::SystemEvent(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, bool autoclear) : state(SystemEventState::Uninitialized)  {
        this->AttachHandles(read_handle, manage_read_handle, write_handle, manage_write_handle, autoclear);
    }

    SystemEvent::~SystemEvent() {
        this->Finalize();
    }

    Event &SystemEvent::GetEvent() {
        AMS_ASSERT(this->state == SystemEventState::Event);
        return GetReference(this->storage_for_event);
    }

    const Event &SystemEvent::GetEvent() const {
        AMS_ASSERT(this->state == SystemEventState::Event);
        return GetReference(this->storage_for_event);
    }

    impl::InterProcessEvent &SystemEvent::GetInterProcessEvent() {
        AMS_ASSERT(this->state == SystemEventState::InterProcessEvent);
        return GetReference(this->storage_for_inter_process_event);
    }

    const impl::InterProcessEvent &SystemEvent::GetInterProcessEvent() const {
        AMS_ASSERT(this->state == SystemEventState::InterProcessEvent);
        return GetReference(this->storage_for_inter_process_event);
    }

    Result SystemEvent::InitializeAsEvent(bool autoclear) {
        AMS_ASSERT(this->state == SystemEventState::Uninitialized);
        new (GetPointer(this->storage_for_event)) Event(autoclear);
        this->state = SystemEventState::Event;
        return ResultSuccess();
    }

    Result SystemEvent::InitializeAsInterProcessEvent(bool autoclear) {
        AMS_ASSERT(this->state == SystemEventState::Uninitialized);
        new (GetPointer(this->storage_for_inter_process_event)) impl::InterProcessEvent();
        this->state = SystemEventState::InterProcessEvent;

        /* Ensure we end up in a correct state if initialization fails. */
        {
            auto guard = SCOPE_GUARD { this->Finalize(); };
            R_TRY(this->GetInterProcessEvent().Initialize(autoclear));
            guard.Cancel();
        }

        return ResultSuccess();
    }

    void SystemEvent::AttachHandles(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, bool autoclear) {
        AMS_ASSERT(this->state == SystemEventState::Uninitialized);
        new (GetPointer(this->storage_for_inter_process_event)) impl::InterProcessEvent();
        this->state = SystemEventState::InterProcessEvent;
        this->GetInterProcessEvent().Initialize(read_handle, manage_read_handle, write_handle, manage_write_handle, autoclear);
    }

    void SystemEvent::AttachReadableHandle(Handle read_handle, bool manage_read_handle, bool autoclear) {
        this->AttachHandles(read_handle, manage_read_handle, INVALID_HANDLE, false, autoclear);
    }

    void SystemEvent::AttachWritableHandle(Handle write_handle, bool manage_write_handle, bool autoclear) {
        this->AttachHandles(INVALID_HANDLE, false, write_handle, manage_write_handle, autoclear);
    }

    Handle SystemEvent::DetachReadableHandle() {
        AMS_ASSERT(this->state == SystemEventState::InterProcessEvent);
        return this->GetInterProcessEvent().DetachReadableHandle();
    }

    Handle SystemEvent::DetachWritableHandle() {
        AMS_ASSERT(this->state == SystemEventState::InterProcessEvent);
        return this->GetInterProcessEvent().DetachWritableHandle();
    }

    Handle SystemEvent::GetReadableHandle() const {
        AMS_ASSERT(this->state == SystemEventState::InterProcessEvent);
        return this->GetInterProcessEvent().GetReadableHandle();
    }

    Handle SystemEvent::GetWritableHandle() const {
        AMS_ASSERT(this->state == SystemEventState::InterProcessEvent);
        return this->GetInterProcessEvent().GetWritableHandle();
    }

    void SystemEvent::Finalize() {
        switch (this->state) {
            case SystemEventState::Uninitialized:
                break;
            case SystemEventState::Event:
                this->GetEvent().~Event();
                break;
            case SystemEventState::InterProcessEvent:
                this->GetInterProcessEvent().~InterProcessEvent();
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
        this->state = SystemEventState::Uninitialized;
    }

    void SystemEvent::Signal() {
        switch (this->state) {
            case SystemEventState::Event:
                this->GetEvent().Signal();
                break;
            case SystemEventState::InterProcessEvent:
                this->GetInterProcessEvent().Signal();
                break;
            case SystemEventState::Uninitialized:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void SystemEvent::Reset() {
        switch (this->state) {
            case SystemEventState::Event:
                this->GetEvent().Reset();
                break;
            case SystemEventState::InterProcessEvent:
                this->GetInterProcessEvent().Reset();
                break;
            case SystemEventState::Uninitialized:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }
    void SystemEvent::Wait() {
        switch (this->state) {
            case SystemEventState::Event:
                this->GetEvent().Wait();
                break;
            case SystemEventState::InterProcessEvent:
                this->GetInterProcessEvent().Wait();
                break;
            case SystemEventState::Uninitialized:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool SystemEvent::TryWait() {
        switch (this->state) {
            case SystemEventState::Event:
                return this->GetEvent().TryWait();
            case SystemEventState::InterProcessEvent:
                return this->GetInterProcessEvent().TryWait();
            case SystemEventState::Uninitialized:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool SystemEvent::TimedWait(u64 ns) {
        switch (this->state) {
            case SystemEventState::Event:
                return this->GetEvent().TimedWait(ns);
            case SystemEventState::InterProcessEvent:
                return this->GetInterProcessEvent().TimedWait(ns);
            case SystemEventState::Uninitialized:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }
}
