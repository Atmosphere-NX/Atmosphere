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
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/ldr.hpp>
#include <stratosphere/pm.hpp>

#include "pm_process_manager.hpp"

namespace sts::pm::impl {

    class ProcessInfo {
        NON_COPYABLE(ProcessInfo);
        private:
            enum Flag : u32 {
                Flag_SignalOnExit           = (1 << 0),
                Flag_ExceptionOccurred      = (1 << 1),
                Flag_ExceptionWaitingAttach = (1 << 2),
                Flag_SignalOnDebugEvent     = (1 << 3),
                Flag_SuspendedStateChanged  = (1 << 4),
                Flag_Suspended              = (1 << 5),
                Flag_Application            = (1 << 6),
                Flag_SignalOnStart          = (1 << 7),
                Flag_StartedStateChanged    = (1 << 8),
            };
        private:
            const u64 process_id;
            const ldr::PinId pin_id;
            const ncm::TitleLocation loc;
            Handle handle;
            ProcessState state;
            u32 flags;
        private:
            void SetFlag(Flag flag) {
                this->flags |= flag;
            }

            void ClearFlag(Flag flag) {
                this->flags &= ~flag;
            }

            bool HasFlag(Flag flag) const {
                return (this->flags & flag);
            }
        public:
            ProcessInfo(Handle h, u64 pid, ldr::PinId pin, const ncm::TitleLocation &l);
            ~ProcessInfo();
            void Cleanup();

            Handle GetHandle() const {
                return this->handle;
            }

            u64 GetProcessId() const {
                return this->process_id;
            }

            ldr::PinId GetPinId() const {
                return this->pin_id;
            }

            const ncm::TitleLocation &GetTitleLocation() {
                return this->loc;
            }

            ProcessState GetState() const {
                return this->state;
            }

            void SetState(ProcessState state) {
                this->state = state;
            }

            void SetSignalOnExit() {
                this->SetFlag(Flag_SignalOnExit);
            }

            void SetExceptionOccurred() {
                this->SetFlag(Flag_ExceptionOccurred);
                this->SetFlag(Flag_ExceptionWaitingAttach);
            }

            void ClearExceptionOccurred() {
                this->ClearFlag(Flag_ExceptionOccurred);
            }

            void ClearExceptionWaitingAttach() {
                this->ClearFlag(Flag_ExceptionWaitingAttach);
            }

            void SetSignalOnDebugEvent() {
                this->SetFlag(Flag_SignalOnDebugEvent);
            }

            void SetSuspendedStateChanged() {
                this->SetFlag(Flag_SuspendedStateChanged);
            }

            void ClearSuspendedStateChanged() {
                this->ClearFlag(Flag_SuspendedStateChanged);
            }

            void SetSuspended() {
                this->SetFlag(Flag_Suspended);
            }

            void ClearSuspended() {
                this->ClearFlag(Flag_Suspended);
            }

            void SetApplication() {
                this->SetFlag(Flag_Application);
            }

            void SetSignalOnStart() {
                this->SetFlag(Flag_SignalOnStart);
            }

            void ClearSignalOnStart() {
                this->ClearFlag(Flag_SignalOnStart);
            }

            void SetStartedStateChanged() {
                this->SetFlag(Flag_StartedStateChanged);
            }

            void ClearStartedStateChanged() {
                this->ClearFlag(Flag_StartedStateChanged);
            }

            bool HasStarted() const {
                return this->state != ProcessState_Created && this->state != ProcessState_CreatedAttached;
            }

            bool HasExited() const {
                return this->state == ProcessState_Exited;
            }

            bool ShouldSignalOnExit() const {
                return this->HasFlag(Flag_SignalOnExit);
            }

            bool HasExceptionOccurred() const {
                return this->HasFlag(Flag_ExceptionOccurred);
            }

            bool HasExceptionWaitingAttach() const {
                return this->HasFlag(Flag_ExceptionWaitingAttach);
            }

            bool ShouldSignalOnDebugEvent() const {
                return this->HasFlag(Flag_SignalOnDebugEvent);
            }

            bool ShouldSignalOnStart() const {
                return this->HasFlag(Flag_SignalOnStart);
            }

            bool HasSuspendedStateChanged() const {
                return this->HasFlag(Flag_SuspendedStateChanged);
            }

            bool IsSuspended() const {
                return this->HasFlag(Flag_Suspended);
            }

            bool IsApplication() const {
                return this->HasFlag(Flag_Application);
            }

            bool HasStartedStateChanged() const {
                return this->HasFlag(Flag_StartedStateChanged);
            }

    };

    Result OnProcessSignaled(std::shared_ptr<ProcessInfo> process_info);

    class ProcessInfoWaiter final : public IWaitable {
        private:
            std::shared_ptr<ProcessInfo> process_info;
        public:
            ProcessInfoWaiter(std::shared_ptr<ProcessInfo> p) : process_info(p) { /* ... */ }

            /* IWaitable */
            Handle GetHandle() override {
                return this->process_info->GetHandle();
            }

            Result HandleSignaled(u64 timeout) override {
                return OnProcessSignaled(this->process_info);
            }
    };

    class ProcessList final {
        private:
            HosMutex lock;
            std::vector<std::shared_ptr<ProcessInfo>> processes;
        public:
            void Lock() {
                this->lock.Lock();
            }

            void Unlock() {
                this->lock.Unlock();
            }

            size_t GetSize() const {
                return this->processes.size();
            }

            std::shared_ptr<ProcessInfo> Pop() {
                auto front = this->processes[0];
                this->processes.erase(this->processes.begin());
                return front;
            }

            void Add(std::shared_ptr<ProcessInfo> process_info) {
                this->processes.push_back(process_info);
            }

            void Remove(u64 process_id) {
                for (size_t i = 0; i < this->processes.size(); i++) {
                    if (this->processes[i]->GetProcessId() == process_id) {
                        this->processes.erase(this->processes.begin() + i);
                        break;
                    }
                }
            }

            std::shared_ptr<ProcessInfo> Find(u64 process_id) {
                for (size_t i = 0; i < this->processes.size(); i++) {
                    if (this->processes[i]->GetProcessId() == process_id) {
                        return this->processes[i];
                    }
                }
                return nullptr;
            }

            std::shared_ptr<ProcessInfo> Find(ncm::TitleId title_id) {
                for (size_t i = 0; i < this->processes.size(); i++) {
                    if (this->processes[i]->GetTitleLocation().title_id == title_id) {
                        return this->processes[i];
                    }
                }
                return nullptr;
            }

            std::shared_ptr<ProcessInfo> operator[](int i) {
                return this->processes[i];
            }

            const std::shared_ptr<ProcessInfo> operator[](int i) const {
                return this->processes[i];
            }
    };

    class ProcessListAccessor final {
        private:
            ProcessList &list;
        public:
            explicit ProcessListAccessor(ProcessList &l) : list(l) {
                this->list.Lock();
            }

            ~ProcessListAccessor() {
                this->list.Unlock();
            }

            ProcessList *operator->() {
                return &this->list;
            }

            const ProcessList *operator->() const {
                return &this->list;
            }

            ProcessList &operator*() {
                return this->list;
            }

            const ProcessList &operator*() const {
                return this->list;
            }

            std::shared_ptr<ProcessInfo> operator[](int i) {
                return this->list[i];
            }

            const std::shared_ptr<ProcessInfo> operator[](int i) const {
                return this->list[i];
            }
    };

}
