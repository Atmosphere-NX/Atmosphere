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
#include <stratosphere.hpp>
#include "dmnt2_debug_log.hpp"
#include "dmnt2_debug_process.hpp"

namespace ams::dmnt {

    Result DebugProcess::Attach(os::ProcessId process_id) {
        /* Attach to the process. */
        R_TRY(svc::DebugActiveProcess(std::addressof(m_debug_handle), process_id.value));

        /* Collect initial information. */
        R_TRY(this->Start());

        /* Get the attached modules. */
        R_TRY(this->CollectModules());

        /* Get our process id. */
        u64 pid_value = 0;
        svc::GetProcessId(std::addressof(pid_value), m_debug_handle);

        m_process_id = { pid_value };
        return ResultSuccess();
    }

    void DebugProcess::Detach() {
        if (m_is_valid) {
            R_ABORT_UNLESS(svc::CloseHandle(m_debug_handle));
            m_debug_handle = svc::InvalidHandle;
        }

        m_is_valid = false;
    }

    Result DebugProcess::Start() {
        /* Process the initial debug events. */
        s32 num_threads = 0;
        bool attached = false;
        while (num_threads == 0 || !attached) {
            /* Wait for debug events to be available. */
            s32 dummy_index;
            R_ABORT_UNLESS(svc::WaitSynchronization(std::addressof(dummy_index), std::addressof(m_debug_handle), 1, svc::WaitInfinite));

            /* Get debug event. */
            svc::DebugEventInfo d;
            R_ABORT_UNLESS(svc::GetDebugEvent(std::addressof(d), m_debug_handle));

            /* Handle the debug event. */
            switch (d.type) {
                case svc::DebugEvent_CreateProcess:
                    {
                        /* Set our create process info. */
                        m_create_process_info = d.info.create_process;

                        /* Cache our bools. */
                        m_is_64_bit               = (m_create_process_info.flags & svc::CreateProcessFlag_Is64Bit);
                        m_is_64_bit_address_space = (m_create_process_info.flags & svc::CreateProcessFlag_AddressSpaceMask) == svc::CreateProcessFlag_AddressSpace64Bit;
                    }
                    break;
                case svc::DebugEvent_CreateThread:
                    {
                        ++num_threads;

                        if (const s32 index = this->ThreadCreate(d.thread_id); index >= 0) {
                            const Result result = osdbg::InitializeThreadInfo(std::addressof(m_thread_infos[index]), m_debug_handle, std::addressof(m_create_process_info), std::addressof(d.info.create_thread));

                            if (R_FAILED(result)) {
                                AMS_DMNT2_GDB_LOG_WARN("DebugProcess::Start: InitializeThreadInfo(%lx) failed: %08x\n", d.thread_id, result.GetValue());
                            }
                        }
                    }
                    break;
                case svc::DebugEvent_ExitThread:
                    {
                        --num_threads;
                        this->ThreadExit(d.thread_id);
                    }
                    break;
                case svc::DebugEvent_Exception:
                    {
                        if (d.info.exception.type == svc::DebugException_DebuggerAttached) {
                            attached = true;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        /* Set ourselves as valid. */
        m_is_valid = true;
        this->SetDebugBreaked();

        return ResultSuccess();
    }

    s32 DebugProcess::ThreadCreate(u64 thread_id) {
        for (size_t i = 0; i < ThreadCountMax; ++i) {
            if (!m_thread_valid[i]) {
                m_thread_valid[i] = true;
                m_thread_ids[i]   = thread_id;

                this->SetLastThreadId(thread_id);
                this->SetLastSignal(GdbSignal_BreakpointTrap);

                ++m_thread_count;

                return i;
            }
        }

        return -1;
    }

    void DebugProcess::ThreadExit(u64 thread_id) {
        for (size_t i = 0; i < ThreadCountMax; ++i) {
            if (m_thread_valid[i] && m_thread_ids[i] == thread_id) {
                m_thread_valid[i] = false;
                m_thread_ids[i]   = 0;

                this->SetLastThreadId(thread_id);
                this->SetLastSignal(GdbSignal_BreakpointTrap);

                --m_thread_count;
                break;
            }
        }
    }

    Result DebugProcess::CollectModules() {
        /* Traverse the address space, looking for modules. */
        uintptr_t address = 0;

        while (true) {
            /* Query the current address. */
            svc::MemoryInfo memory_info;
            svc::PageInfo page_info;
            if (R_SUCCEEDED(svc::QueryDebugProcessMemory(std::addressof(memory_info), std::addressof(page_info), m_debug_handle, address))) {
                if (memory_info.perm == svc::MemoryPermission_ReadExecute && (memory_info.state == svc::MemoryState_Code || memory_info.state == svc::MemoryState_AliasCode)) {
                    /* Check that we can add the module. */
                    AMS_ABORT_UNLESS(m_module_count < ModuleCountMax);

                    /* Get module definition. */
                    auto &module = m_module_definitions[m_module_count++];

                    /* Set module address/size. */
                    module.SetAddressSize(memory_info.addr, memory_info.size);

                    /* Get module name buffer. */
                    char *module_name = module.GetNameBuffer();
                    module_name[0] = 0;

                    /* Read module path. */
                    struct {
                        u32  zero;
                        s32  path_length;
                        char path[ModuleDefinition::PathLengthMax];
                    } module_path;
                    if (R_SUCCEEDED(this->ReadMemory(std::addressof(module_path), memory_info.addr + memory_info.size, sizeof(module_path)))) {
                        if (module_path.zero == 0 && module_path.path_length == util::Strnlen(module_path.path, sizeof(module_path.path))) {
                            std::memcpy(module_name, module_path.path, ModuleDefinition::PathLengthMax);
                        }
                    }

                    /* Truncate module name. */
                    module_name[ModuleDefinition::PathLengthMax - 1] = 0;
                }
            }

            /* Check if we're done. */
            const uintptr_t next_address = memory_info.addr + memory_info.size;
            if (memory_info.state == svc::MemoryState_Inaccessible) {
                break;
            }
            if (next_address <= address) {
                break;
            }

            address = next_address;
        }

        return ResultSuccess();
    }

    Result DebugProcess::GetThreadContext(svc::ThreadContext *out, u64 thread_id, u32 flags) {
        return svc::GetDebugThreadContext(out, m_debug_handle, thread_id, flags);
    }

    Result DebugProcess::SetThreadContext(const svc::ThreadContext *ctx, u64 thread_id, u32 flags) {
        return svc::SetDebugThreadContext(m_debug_handle, thread_id, ctx, flags);
    }

    Result DebugProcess::ReadMemory(void *dst, uintptr_t address, size_t size) {
        return svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(dst), m_debug_handle, address, size);
    }

    Result DebugProcess::WriteMemory(const void *src, uintptr_t address, size_t size) {
        return svc::WriteDebugProcessMemory(m_debug_handle, reinterpret_cast<uintptr_t>(src), address, size);
    }

    Result DebugProcess::GetThreadCurrentCore(u32 *out, u64 thread_id) {
        u64 dummy_value;
        u32 val32 = 0;

        R_TRY(svc::GetDebugThreadParam(std::addressof(dummy_value), std::addressof(val32), m_debug_handle, thread_id, svc::DebugThreadParam_CurrentCore));

        *out = val32;
        return ResultSuccess();
    }

    Result DebugProcess::GetProcessDebugEvent(svc::DebugEventInfo *out) {
        /* Get the event. */
        R_TRY(svc::GetDebugEvent(out, m_debug_handle));

        /* Process the event. */
        switch (out->type) {
            case svc::DebugEvent_CreateProcess:
                {
                    /* Set our create process info. */
                    m_create_process_info = out->info.create_process;

                    /* Cache our bools. */
                    m_is_64_bit               = (m_create_process_info.flags & svc::CreateProcessFlag_Is64Bit);
                    m_is_64_bit_address_space = (m_create_process_info.flags & svc::CreateProcessFlag_AddressSpaceMask) == svc::CreateProcessFlag_AddressSpace64Bit;
                }
                break;
            case svc::DebugEvent_CreateThread:
                {
                    if (const s32 index = this->ThreadCreate(out->thread_id); index >= 0) {
                        const Result result = osdbg::InitializeThreadInfo(std::addressof(m_thread_infos[index]), m_debug_handle, std::addressof(m_create_process_info), std::addressof(out->info.create_thread));

                        if (R_FAILED(result)) {
                            AMS_DMNT2_GDB_LOG_WARN("DebugProcess::GetProcessDebugEvent: InitializeThreadInfo(%lx) failed: %08x\n", out->thread_id, result.GetValue());
                        }
                    }
                }
                break;
            case svc::DebugEvent_ExitThread:
                {
                    this->ThreadExit(out->thread_id);
                }
                break;
            default:
                break;
        }

        if (out->flags & svc::DebugEventFlag_Stopped) {
            this->SetDebugBreaked();
        }

        return ResultSuccess();
    }

    u64 DebugProcess::GetLastThreadId() {
        /* Select our first valid thread id. */
        if (m_last_thread_id == 0) {
            for (size_t i = 0; i < ThreadCountMax; ++i) {
                if (m_thread_valid[i]) {
                    SetLastThreadId(m_thread_ids[i]);
                    break;
                }
            }
        }

        return m_last_thread_id;
    }

    Result DebugProcess::GetThreadList(s32 *out_count, u64 *out_thread_ids, size_t max_count) {
        s32 count = 0;
        for (size_t i = 0; i < ThreadCountMax; ++i) {
            if (m_thread_valid[i]) {
                if (count < static_cast<s32>(max_count)) {
                    out_thread_ids[count++] = m_thread_ids[i];
                }
            }
        }

        *out_count = count;
        return ResultSuccess();
    }

    Result DebugProcess::GetThreadInfoList(s32 *out_count, osdbg::ThreadInfo **out_infos, size_t max_count) {
        s32 count = 0;
        for (size_t i = 0; i < ThreadCountMax; ++i) {
            if (m_thread_valid[i]) {
                if (count < static_cast<s32>(max_count)) {
                    out_infos[count++] = std::addressof(m_thread_infos[i]);
                }
            }
        }

        *out_count = count;
        return ResultSuccess();
    }


}