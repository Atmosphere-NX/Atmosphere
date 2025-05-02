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
#include "pm_process_manager.hpp"
#include "pm_process_attributes.hpp"

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
            util::IntrusiveListNode m_list_node;
            const os::ProcessId m_process_id;
            const ldr::PinId m_pin_id;
            const ncm::ProgramLocation m_loc;
            const cfg::OverrideStatus m_status;
            os::NativeHandle m_handle;
            svc::ProcessState m_state;
            u32 m_flags;
            ProcessAttributes m_attrs;
            os::MultiWaitHolderType m_multi_wait_holder;
        private:
            void SetFlag(Flag flag) {
                m_flags |= flag;
            }

            void ClearFlag(Flag flag) {
                m_flags &= ~flag;
            }

            bool HasFlag(Flag flag) const {
                return (m_flags & flag);
            }
        public:
            ProcessInfo(os::NativeHandle h, os::ProcessId pid, ldr::PinId pin, const ncm::ProgramLocation &l, const cfg::OverrideStatus &s, const ProcessAttributes &attrs);
            ~ProcessInfo();
            void Cleanup();

            os::MultiWaitHolderType *GetMultiWaitHolder() {
                return std::addressof(m_multi_wait_holder);
            }

            os::NativeHandle GetHandle() const {
                return m_handle;
            }

            os::ProcessId GetProcessId() const {
                return m_process_id;
            }

            ldr::PinId GetPinId() const {
                return m_pin_id;
            }

            const ncm::ProgramLocation &GetProgramLocation() const {
                return m_loc;
            }

            const cfg::OverrideStatus &GetOverrideStatus() const {
                return m_status;
            }

            const ProcessAttributes &GetProcessAttributes() const {
                return m_attrs;
            }

            svc::ProcessState GetState() const {
                return m_state;
            }

            void SetState(svc::ProcessState state) {
                m_state = state;
            }

            bool HasStarted() const {
                return m_state != svc::ProcessState_Created && m_state != svc::ProcessState_CreatedAttached;
            }

            bool HasTerminated() const {
                return m_state == svc::ProcessState_Terminated;
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

    class ProcessList final : public util::IntrusiveListMemberTraits<&ProcessInfo::m_list_node>::ListType {
        private:
            os::SdkMutex m_lock;
        public:
            constexpr ProcessList() : m_lock() { /* ... */ }

            void Lock() {
                m_lock.Lock();
            }

            void Unlock() {
                m_lock.Unlock();
            }

            void Remove(ProcessInfo *process_info) {
                this->erase(this->iterator_to(*process_info));
            }

            ProcessInfo *Find(os::ProcessId process_id) {
                for (auto &info : *this) {
                    if (info.GetProcessId() == process_id) {
                        return std::addressof(info);
                    }
                }
                return nullptr;
            }

            ProcessInfo *Find(ncm::ProgramId program_id) {
                for (auto &info : *this) {
                    if (info.GetProgramLocation().program_id == program_id) {
                        return std::addressof(info);
                    }
                }
                return nullptr;
            }

    };

    class ProcessListAccessor final {
        private:
            ProcessList &m_list;
        public:
            explicit ProcessListAccessor(ProcessList &l) : m_list(l) {
                m_list.Lock();
            }

            ~ProcessListAccessor() {
                m_list.Unlock();
            }

            ProcessList *operator->() {
                return std::addressof(m_list);
            }

            const ProcessList *operator->() const {
                return std::addressof(m_list);
            }

            ProcessList &operator*() {
                return m_list;
            }

            const ProcessList &operator*() const {
                return m_list;
            }
    };

    ProcessListAccessor GetProcessList();
    ProcessListAccessor GetExitList();

    ProcessInfo *AllocateProcessInfo(svc::Handle process_handle, os::ProcessId process_id, ldr::PinId pin_id, const ncm::ProgramLocation &location, const cfg::OverrideStatus &override_status, const ProcessAttributes &attrs);
    void CleanupProcessInfo(ProcessListAccessor &list, ProcessInfo *process_info);

}
