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
            DebugEventList event_info_list;
            u32 continue_flags;
            KProcess *process;
            KLightLock lock;
            KProcess::State old_process_state;
        public:
            explicit KDebugBase() { /* ... */ }
            virtual ~KDebugBase() { /* ... */ }
        public:
            void Initialize();
            Result Attach(KProcess *process);

            Result QueryMemoryInfo(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info, KProcessAddress address);

            Result GetDebugEventInfo(ams::svc::lp64::DebugEventInfo *out);
            Result GetDebugEventInfo(ams::svc::ilp32::DebugEventInfo *out);

            /* TODO: This is a placeholder definition. */
        private:
            KScopedAutoObject<KProcess> GetProcess();

            void PushDebugEvent(ams::svc::DebugEvent event, uintptr_t param0 = 0, uintptr_t param1 = 0, uintptr_t param2 = 0, uintptr_t param3 = 0, uintptr_t param4 = 0);
            void EnqueueDebugEventInfo(KEventInfo *info);
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
