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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_process.hpp>
#include <mesosphere/kern_k_event_info.hpp>
#include <mesosphere/kern_k_light_lock.hpp>

namespace ams::kern {

    class KDebugBase : public KSynchronizationObject {
        protected:
            using DebugEventList = util::IntrusiveListBaseTraits<KEventInfo>::ListType;
        private:
            DebugEventList m_event_info_list;
            u32 m_continue_flags;
            KProcess *m_process;
            KLightLock m_lock;
            KProcess::State m_old_process_state;
        public:
            explicit KDebugBase() { /* ... */ }
            virtual ~KDebugBase() { /* ... */ }
        protected:
            bool Is64Bit() const;
        public:
            void Initialize();

            Result Attach(KProcess *process);
            Result BreakProcess();
            Result TerminateProcess();

            Result ContinueDebug(const u32 flags, const u64 *thread_ids, size_t num_thread_ids);

            Result QueryMemoryInfo(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info, KProcessAddress address);
            Result ReadMemory(KProcessAddress buffer, KProcessAddress address, size_t size);
            Result WriteMemory(KProcessAddress buffer, KProcessAddress address, size_t size);

            Result GetThreadContext(ams::svc::ThreadContext *out, u64 thread_id, u32 context_flags);
            Result SetThreadContext(const ams::svc::ThreadContext &ctx, u64 thread_id, u32 context_flags);

            virtual Result GetThreadContextImpl(ams::svc::ThreadContext *out, KThread *thread, u32 context_flags) = 0;
            virtual Result SetThreadContextImpl(const ams::svc::ThreadContext &ctx, KThread *thread, u32 context_flags) = 0;

            Result GetRunningThreadInfo(ams::svc::LastThreadContext *out_context, u64 *out_thread_id);

            Result GetDebugEventInfo(ams::svc::lp64::DebugEventInfo *out);
            Result GetDebugEventInfo(ams::svc::ilp32::DebugEventInfo *out);

            KScopedAutoObject<KProcess> GetProcess();
        private:
            void PushDebugEvent(ams::svc::DebugEvent event, uintptr_t param0 = 0, uintptr_t param1 = 0, uintptr_t param2 = 0, uintptr_t param3 = 0, uintptr_t param4 = 0);
            void EnqueueDebugEventInfo(KEventInfo *info);

            template<typename T> requires (std::same_as<T, ams::svc::lp64::DebugEventInfo> || std::same_as<T, ams::svc::ilp32::DebugEventInfo>)
            Result GetDebugEventInfoImpl(T *out);
        public:
            virtual void OnFinalizeSynchronizationObject() override;
            virtual bool IsSignaled() const override;
        private:
            static Result ProcessDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4);
        public:
            static Result OnDebugEvent(ams::svc::DebugEvent event, uintptr_t param0 = 0, uintptr_t param1 = 0, uintptr_t param2 = 0, uintptr_t param3 = 0, uintptr_t param4 = 0);
            static Result OnExitProcess(KProcess *process);
            static Result OnTerminateProcess(KProcess *process);
            static Result OnExitThread(KThread *thread);
            static KEventInfo *CreateDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4, u64 thread_id);
    };

}
