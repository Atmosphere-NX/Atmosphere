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
#include <stratosphere.hpp>
#include "dmnt2_gdb_signal.hpp"
#include "dmnt2_module_definition.hpp"

namespace ams::dmnt {

    class DebugProcess {
        public:
            static constexpr size_t ThreadCountMax = 0x100;
            static constexpr size_t ModuleCountMax = 0x60;

            enum ProcessStatus {
                ProcessStatus_DebugBreak,
                ProcessStatus_Running,
                ProcessStatus_Exited,
            };

            enum ContinueMode {
                ContinueMode_Stopped,
                ContinueMode_Continue,
                ContinueMode_Step,
            };
        private:
            svc::Handle m_debug_handle{svc::InvalidHandle};
            s32 m_thread_count{0};
            bool m_is_valid{false};
            bool m_is_64_bit{false};
            bool m_is_64_bit_address_space{false};
            ProcessStatus m_status{ProcessStatus_DebugBreak};
            os::ProcessId m_process_id{os::InvalidProcessId};
            u64 m_last_thread_id{};
            u64 m_thread_id_override;
            GdbSignal m_last_signal{};
            bool m_thread_valid[ThreadCountMax]{};
            u64 m_thread_ids[ThreadCountMax]{};
            osdbg::ThreadInfo m_thread_infos[ThreadCountMax]{};
            svc::DebugInfoCreateProcess m_create_process_info{};
            ModuleDefinition m_module_definitions[ModuleCountMax]{};
            size_t m_module_count{};
            size_t m_main_module{};
        public:
            constexpr DebugProcess() = default;
            ~DebugProcess() { this->Detach(); }

            svc::Handle GetHandle() const { return m_debug_handle; }
            bool IsValid() const { return m_is_valid; }
            bool Is64Bit() const { return m_is_64_bit; }
            bool Is64BitAddressSpace() const { return m_is_64_bit_address_space; }
            size_t GetModuleCount() const { return m_module_count; }
            size_t GetMainModuleIndex() const { return m_main_module; }
            const char *GetModuleName(size_t ix) const { return m_module_definitions[ix].GetName(); }
            uintptr_t GetBaseAddress(size_t ix) const { return m_module_definitions[ix].GetAddress(); }
            ProcessStatus GetStatus() const { return m_status; }
            os::ProcessId GetProcessId() const { return m_process_id; }

            const char *GetProcessName() const { return m_create_process_info.name; }

            void SetLastSignal(GdbSignal signal) { m_last_signal = signal; }
            GdbSignal GetLastSignal() const { return m_last_signal; }

            Result GetThreadList(s32 *out_count, u64 *out_thread_ids, size_t max_count);
            Result GetThreadInfoList(s32 *out_count, osdbg::ThreadInfo **out_infos, size_t max_count);

            u64 GetLastThreadId();
            u64 GetThreadIdOverride() { this->GetLastThreadId(); return m_thread_id_override; }

            void SetLastThreadId(u64 tid) {
                m_last_thread_id     = tid;
                m_thread_id_override = tid;
            }

            void SetThreadIdOverride(u64 tid) {
                m_thread_id_override = tid;
            }

            void SetDebugBreaked() {
                m_status = ProcessStatus_DebugBreak;
            }
        public:
            Result Attach(os::ProcessId process_id);
            void Detach();

            Result GetThreadContext(svc::ThreadContext *out, u64 thread_id, u32 flags);
            Result SetThreadContext(const svc::ThreadContext *ctx, u64 thread_id, u32 flags);

            Result ReadMemory(void *dst, uintptr_t address, size_t size);
            Result WriteMemory(const void *src, uintptr_t address, size_t size);

            Result GetThreadCurrentCore(u32 *out, u64 thread_id);

            Result GetProcessDebugEvent(svc::DebugEventInfo *out);
        private:
            Result Start();

            s32 ThreadCreate(u64 thread_id);
            void ThreadExit(u64 thread_id);

            Result CollectModules();
    };

}