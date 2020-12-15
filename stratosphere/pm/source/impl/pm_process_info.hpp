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
#include "pm_process_manager.hpp"

namespace ams::pm::impl {

    class ProcessList;

    class ProcessInfo {
        friend class ProcessList;
        NON_COPYABLE(ProcessInfo);
        NON_MOVEABLE(ProcessInfo);
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
                Flag_UnhandledException     = (1 << 9),
            };
        private:
            util::IntrusiveListNode list_node;
            const os::ProcessId process_id;
            const ldr::PinId pin_id;
            const ncm::ProgramLocation loc;
            const cfg::OverrideStatus status;
            Handle handle;
            svc::ProcessState state;
            u32 flags;
            os::WaitableHolderType waitable_holder;
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
            ProcessInfo(Handle h, os::ProcessId pid, ldr::PinId pin, const ncm::ProgramLocation &l, const cfg::OverrideStatus &s);
            ~ProcessInfo();
            void Cleanup();

            void LinkToWaitableManager(os::WaitableManagerType &manager) {
                os::LinkWaitableHolder(std::addressof(manager), std::addressof(this->waitable_holder));
            }

            Handle GetHandle() const {
                return this->handle;
            }

            os::ProcessId GetProcessId() const {
                return this->process_id;
            }

            ldr::PinId GetPinId() const {
                return this->pin_id;
            }

            const ncm::ProgramLocation &GetProgramLocation() const {
                return this->loc;
            }

            const cfg::OverrideStatus &GetOverrideStatus() const {
                return this->status;
            }

            svc::ProcessState GetState() const {
                return this->state;
            }

            void SetState(svc::ProcessState state) {
                this->state = state;
            }

            bool HasStarted() const {
                return this->state != svc::ProcessState_Created && this->state != svc::ProcessState_CreatedAttached;
            }

            bool HasTerminated() const {
                return this->state == svc::ProcessState_Terminated;
            }

#define DEFINE_FLAG_SET(flag) \
            void Set##flag() { \
                this->SetFlag(Flag_##flag); \
            }

#define DEFINE_FLAG_GET(get, flag) \
            bool get##flag() const { \
                return this->HasFlag(Flag_##flag); \
            }

#define DEFINE_FLAG_CLEAR(flag) \
            void Clear##flag() { \
                this->ClearFlag(Flag_##flag); \
            }

            DEFINE_FLAG_SET(SignalOnExit)
            DEFINE_FLAG_GET(Should, SignalOnExit)

            /* This needs a manual setter, because it sets two flags. */
            void SetExceptionOccurred() {
                this->SetFlag(Flag_ExceptionOccurred);
                this->SetFlag(Flag_UnhandledException);
            }

            DEFINE_FLAG_GET(Has, ExceptionOccurred)
            DEFINE_FLAG_GET(Has, ExceptionWaitingAttach)
            DEFINE_FLAG_GET(Has, UnhandledException)

            DEFINE_FLAG_SET(ExceptionWaitingAttach)

            DEFINE_FLAG_CLEAR(ExceptionOccurred)
            DEFINE_FLAG_CLEAR(ExceptionWaitingAttach)
            DEFINE_FLAG_CLEAR(UnhandledException)

            DEFINE_FLAG_SET(SignalOnDebugEvent)
            DEFINE_FLAG_GET(Should, SignalOnDebugEvent)

            DEFINE_FLAG_SET(SuspendedStateChanged)
            DEFINE_FLAG_GET(Has, SuspendedStateChanged)
            DEFINE_FLAG_CLEAR(SuspendedStateChanged)

            DEFINE_FLAG_SET(Suspended)
            DEFINE_FLAG_GET(Is, Suspended)
            DEFINE_FLAG_CLEAR(Suspended)

            DEFINE_FLAG_SET(Application)
            DEFINE_FLAG_GET(Is, Application)

            DEFINE_FLAG_SET(SignalOnStart)
            DEFINE_FLAG_GET(Should, SignalOnStart)
            DEFINE_FLAG_CLEAR(SignalOnStart)

            DEFINE_FLAG_SET(StartedStateChanged)
            DEFINE_FLAG_GET(Has, StartedStateChanged)
            DEFINE_FLAG_CLEAR(StartedStateChanged)

#undef DEFINE_FLAG_SET
#undef DEFINE_FLAG_GET
#undef DEFINE_FLAG_CLEAR
    };

    class ProcessList final : public util::IntrusiveListMemberTraits<&ProcessInfo::list_node>::ListType {
        private:
            os::Mutex lock;
        public:
            constexpr ProcessList() : lock(false) { /* ... */ }

            void Lock() {
                this->lock.Lock();
            }

            void Unlock() {
                this->lock.Unlock();
            }

            void Remove(ProcessInfo *process_info) {
                this->erase(this->iterator_to(*process_info));
            }

            ProcessInfo *Find(os::ProcessId process_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if ((*it).GetProcessId() == process_id) {
                        return &*it;
                    }
                }
                return nullptr;
            }

            ProcessInfo *Find(ncm::ProgramId program_id) {
                for (auto it = this->begin(); it != this->end(); it++) {
                    if ((*it).GetProgramLocation().program_id == program_id) {
                        return &*it;
                    }
                }
                return nullptr;
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
    };

}
