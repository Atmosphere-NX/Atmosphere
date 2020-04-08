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
#include <stratosphere/os/os_event_common.hpp>
#include <stratosphere/os/os_system_event_types.hpp>
#include <stratosphere/os/os_system_event_api.hpp>

namespace ams::os {

    class SystemEvent {
        NON_COPYABLE(SystemEvent);
        NON_MOVEABLE(SystemEvent);
        private:
            SystemEventType system_event;
        public:
            SystemEvent() {
                this->system_event.state = SystemEventType::State_NotInitialized;
            }

            explicit SystemEvent(EventClearMode clear_mode, bool inter_process) {
                R_ABORT_UNLESS(CreateSystemEvent(std::addressof(this->system_event), clear_mode, inter_process));
            }

            explicit SystemEvent(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, EventClearMode clear_mode) {
                AttachSystemEvent(std::addressof(this->system_event), read_handle, manage_read_handle, write_handle, manage_write_handle, clear_mode);
            }

            ~SystemEvent() {
                if (this->system_event.state == SystemEventType::State_NotInitialized) {
                    return;
                }
                DestroySystemEvent(std::addressof(this->system_event));
            }

            void Attach(Handle read_handle, bool manage_read_handle, Handle write_handle, bool manage_write_handle, EventClearMode clear_mode) {
                AMS_ABORT_UNLESS(this->system_event.state == SystemEventType::State_NotInitialized);
                return AttachSystemEvent(std::addressof(this->system_event), read_handle, manage_read_handle, write_handle, manage_write_handle, clear_mode);
            }

            void AttachReadableHandle(Handle read_handle, bool manage_read_handle, EventClearMode clear_mode) {
                AMS_ABORT_UNLESS(this->system_event.state == SystemEventType::State_NotInitialized);
                return AttachReadableHandleToSystemEvent(std::addressof(this->system_event), read_handle, manage_read_handle, clear_mode);
            }

            void AttachWritableHandle(Handle write_handle, bool manage_write_handle, EventClearMode clear_mode) {
                AMS_ABORT_UNLESS(this->system_event.state == SystemEventType::State_NotInitialized);
                return AttachWritableHandleToSystemEvent(std::addressof(this->system_event), write_handle, manage_write_handle, clear_mode);
            }

            Handle DetachReadableHandle() {
                return DetachReadableHandleOfSystemEvent(std::addressof(this->system_event));
            }

            Handle DetachWritableHandle() {
                return DetachWritableHandleOfSystemEvent(std::addressof(this->system_event));
            }

            void Wait() {
                return WaitSystemEvent(std::addressof(this->system_event));
            }

            bool TryWait() {
                return TryWaitSystemEvent(std::addressof(this->system_event));
            }

            bool TimedWait(TimeSpan timeout) {
                return TimedWaitSystemEvent(std::addressof(this->system_event), timeout);
            }

            void Signal() {
                return SignalSystemEvent(std::addressof(this->system_event));
            }

            void Clear() {
                return ClearSystemEvent(std::addressof(this->system_event));
            }

            Handle GetReadableHandle() const {
                return GetReadableHandleOfSystemEvent(std::addressof(this->system_event));
            }

            Handle GetWritableHandle() const {
                return GetWritableHandleOfSystemEvent(std::addressof(this->system_event));
            }

            operator SystemEventType &() {
                return this->system_event;
            }

            operator const SystemEventType &() const {
                return this->system_event;
            }

            SystemEventType *GetBase() {
                return std::addressof(this->system_event);
            }
    };

}
