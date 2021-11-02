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
#include <stratosphere.hpp>
#include "dmnt2_gdb_signal.hpp"
#include "dmnt2_module_definition.hpp"
#include "dmnt2_software_breakpoint.hpp"
#include "dmnt2_hardware_breakpoint.hpp"
#include "dmnt2_hardware_watchpoint.hpp"

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
            os::NativeHandle m_debug_handle{os::InvalidNativeHandle};
            s32 m_thread_count{0};
            bool m_is_valid{false};
            bool m_is_64_bit{false};
            bool m_is_64_bit_address_space{false};
            ProcessStatus m_status{ProcessStatus_DebugBreak};
            os::ProcessId m_process_id{os::InvalidProcessId};
            u64 m_last_thread_id{};
            u64 m_thread_id_override{};
            u64 m_continue_thread_id{};
            u64 m_preferred_debug_break_thread_id{};
            GdbSignal m_last_signal{};
            bool m_stepping{false};
            bool m_use_hardware_single_step{false};
            bool m_thread_valid[ThreadCountMax]{};
            u64 m_thread_ids[ThreadCountMax]{};
            osdbg::ThreadInfo m_thread_infos[ThreadCountMax]{};
            svc::DebugInfoCreateProcess m_create_process_info{};
            SoftwareBreakPointManager m_software_breakpoints;
            HardwareBreakPointManager m_hardware_breakpoints;
            HardwareWatchPointManager m_hardware_watchpoints;
            BreakPointManager &m_step_breakpoints;
            ModuleDefinition m_module_definitions[ModuleCountMax]{};
            size_t m_module_count{};
            size_t m_main_module{};
            u64 m_process_alias_address{};
            u64 m_process_alias_size{};
            u64 m_process_heap_address{};
            u64 m_process_heap_size{};
            u64 m_process_aslr_address{};
            u64 m_process_aslr_size{};
            u64 m_process_stack_address{};
            u64 m_process_stack_size{};
            ncm::ProgramLocation m_program_location{};
            cfg::OverrideStatus m_process_override_status{};
            bool m_is_application{false};
        public:
            DebugProcess() : m_software_breakpoints(this), m_hardware_breakpoints(this), m_hardware_watchpoints(this), m_step_breakpoints(m_software_breakpoints) {
                if (svc::IsKernelMesosphere()) {
                    uint64_t value = 0;
                    m_use_hardware_single_step = R_SUCCEEDED(::ams::svc::GetInfo(std::addressof(value), ::ams::svc::InfoType_MesosphereMeta, ::ams::svc::InvalidHandle, ::ams::svc::MesosphereMetaInfo_IsSingleStepEnabled)) && value != 0;
                }
            }
            ~DebugProcess() { this->Detach(); }

            os::NativeHandle GetHandle() const { return m_debug_handle; }
            bool IsValid() const { return m_is_valid; }
            bool Is64Bit() const { return m_is_64_bit; }
            bool Is64BitAddressSpace() const { return m_is_64_bit_address_space; }
            size_t GetModuleCount() const { return m_module_count; }
            size_t GetMainModuleIndex() const { return m_main_module; }
            const char *GetModuleName(size_t ix) const { return m_module_definitions[ix].GetName(); }
            uintptr_t GetModuleBaseAddress(size_t ix) const { return m_module_definitions[ix].GetAddress(); }
            uintptr_t GetModuleSize(size_t ix) const { return m_module_definitions[ix].GetSize(); }
            ProcessStatus GetStatus() const { return m_status; }
            os::ProcessId GetProcessId() const { return m_process_id; }

            const char *GetProcessName() const { return m_create_process_info.name; }

            void SetLastSignal(GdbSignal signal) { m_last_signal = signal; }
            GdbSignal GetLastSignal() const { return m_last_signal; }

            Result GetThreadList(s32 *out_count, u64 *out_thread_ids, size_t max_count);
            Result GetThreadInfoList(s32 *out_count, osdbg::ThreadInfo **out_infos, size_t max_count);

            u64 GetLastThreadId();
            u64 GetThreadIdOverride() { this->GetLastThreadId(); return m_thread_id_override; }

            u64 GetPreferredDebuggerBreakThreadId() { return m_preferred_debug_break_thread_id; }

            void SetLastThreadId(u64 tid) {
                m_last_thread_id     = tid;
                m_thread_id_override = tid;
            }

            void SetThreadIdOverride(u64 tid) {
                m_thread_id_override              = tid;
                m_preferred_debug_break_thread_id = tid;
            }

            void SetDebugBreaked() {
                m_status = ProcessStatus_DebugBreak;
            }

            u64 GetAliasRegionAddress() const { return m_process_alias_address; }
            u64 GetAliasRegionSize()    const { return m_process_alias_size; }
            u64 GetHeapRegionAddress()  const { return m_process_heap_address; }
            u64 GetHeapRegionSize()     const { return m_process_heap_size; }
            u64 GetAslrRegionAddress()  const { return m_process_aslr_address; }
            u64 GetAslrRegionSize()     const { return m_process_aslr_size; }
            u64 GetStackRegionAddress() const { return m_process_stack_address; }
            u64 GetStackRegionSize()    const { return m_process_stack_size; }

            const ncm::ProgramLocation &GetProgramLocation() const { return m_program_location; }
            const cfg::OverrideStatus &GetOverrideStatus() const { return m_process_override_status; }

            bool IsApplication() const { return m_is_application; }
        public:
            Result Attach(os::ProcessId process_id, bool start_process = false);
            void Detach();

            Result GetThreadContext(svc::ThreadContext *out, u64 thread_id, u32 flags);
            Result SetThreadContext(const svc::ThreadContext *ctx, u64 thread_id, u32 flags);

            Result ReadMemory(void *dst, uintptr_t address, size_t size);
            Result WriteMemory(const void *src, uintptr_t address, size_t size);

            Result QueryMemory(svc::MemoryInfo *out, uintptr_t address);

            Result Continue();
            Result Continue(u64 thread_id);
            Result Step();
            Result Step(u64 thread_id);
            void ClearStep();

            Result Break();

            Result Terminate();

            Result SetBreakPoint(uintptr_t address, size_t size, bool is_step);
            Result ClearBreakPoint(uintptr_t address, size_t size);

            Result SetHardwareBreakPoint(uintptr_t address, size_t size, bool is_step);
            Result ClearHardwareBreakPoint(uintptr_t address, size_t size);

            Result SetWatchPoint(u64 address, u64 size, bool read, bool write);
            Result ClearWatchPoint(u64 address, u64 size);
            Result GetWatchPointInfo(u64 address, bool &read, bool &write);

            static bool IsValidWatchPoint(u64 address, u64 size);

            Result GetThreadCurrentCore(u32 *out, u64 thread_id);

            Result GetProcessDebugEvent(svc::DebugEventInfo *out);

            void GetBranchTarget(svc::ThreadContext &ctx, u64 thread_id, u64 &current_pc, u64 &target);

            Result CollectModules();

            void GetThreadName(char *dst, u64 thread_id) const;
        private:
            Result Start();

            void CollectProcessInfo();

            s32 ThreadCreate(u64 thread_id);
            void ThreadExit(u64 thread_id);
    };

}