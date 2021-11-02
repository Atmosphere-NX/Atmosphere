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
#include <stratosphere.hpp>
#include "dmnt2_debug_log.hpp"
#include "dmnt2_debug_process.hpp"

namespace ams::dmnt {

    namespace {

        s32 SignExtend(u32 value, u32 bits) {
            return static_cast<s32>(value << (32 - bits)) >> (32 - bits);
        }

    }

    Result DebugProcess::Attach(os::ProcessId process_id, bool start_process) {
        /* Attach to the process. */
        R_TRY(svc::DebugActiveProcess(std::addressof(m_debug_handle), process_id.value));

        /* If necessary, start the process. */
        if (start_process) {
            R_ABORT_UNLESS(pm::dmnt::StartProcess(process_id));
        }

        /* Collect initial information. */
        R_TRY(this->Start());

        /* Get the attached modules. */
        R_TRY(this->CollectModules());

        /* Get our process id. */
        u64 pid_value = 0;
        svc::GetProcessId(std::addressof(pid_value), m_debug_handle);

        m_process_id = { pid_value };

        /* Get process info. */
        this->CollectProcessInfo();

        return ResultSuccess();
    }

    void DebugProcess::Detach() {
        if (m_is_valid) {
            m_software_breakpoints.ClearAll();
            m_hardware_breakpoints.ClearAll();
            m_hardware_watchpoints.ClearAll();

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
        /* Reset our module count. */
        m_module_count = 0;

        /* Traverse the address space, looking for modules. */
        uintptr_t address = 0;

        while (true) {
            /* Query the current address. */
            svc::MemoryInfo memory_info;
            svc::PageInfo page_info;
            if (R_SUCCEEDED(svc::QueryDebugProcessMemory(std::addressof(memory_info), std::addressof(page_info), m_debug_handle, address))) {
                if (memory_info.permission == svc::MemoryPermission_ReadExecute && (memory_info.state == svc::MemoryState_Code || memory_info.state == svc::MemoryState_AliasCode)) {
                    /* Check that we can add the module. */
                    AMS_ABORT_UNLESS(m_module_count < ModuleCountMax);

                    /* Get module definition. */
                    auto &module = m_module_definitions[m_module_count++];

                    /* Set module address/size. */
                    module.SetAddressSize(memory_info.base_address, memory_info.size);

                    /* Get module name buffer. */
                    char *module_name = module.GetNameBuffer();
                    module_name[0] = 0;

                    /* Read module path. */
                    struct {
                        u32  zero;
                        s32  path_length;
                        char path[ModuleDefinition::PathLengthMax];
                    } module_path;
                    if (R_SUCCEEDED(this->ReadMemory(std::addressof(module_path), memory_info.base_address + memory_info.size, sizeof(module_path)))) {
                        if (module_path.zero == 0 && module_path.path_length > 0) {
                            std::memcpy(module_name, module_path.path, std::min<size_t>(ModuleDefinition::PathLengthMax, module_path.path_length));
                        }
                    } else {
                        module_path.path_length = 0;
                    }

                    /* Truncate module name. */
                    module_name[ModuleDefinition::PathLengthMax - 1] = 0;

                    /* Set default module name start. */
                    module.SetNameStart(0);

                    /* Ignore leading directories. */
                    for (size_t i = 0; i < std::min<size_t>(ModuleDefinition::PathLengthMax, module_path.path_length) && module_name[i] != 0; ++i) {
                        if (module_name[i] == '/' || module_name[i] == '\\') {
                            module.SetNameStart(i + 1);
                        }
                    }
                }
            }

            /* Check if we're done. */
            const uintptr_t next_address = memory_info.base_address + memory_info.size;
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

    void DebugProcess::CollectProcessInfo() {
        /* Define helper for getting process info. */
        auto CollectProcessInfoImpl = [&](os::NativeHandle handle) -> Result {
            /* Collect all values. */
            R_TRY(svc::GetInfo(std::addressof(m_process_alias_address), svc::InfoType_AliasRegionAddress, handle, 0));
            R_TRY(svc::GetInfo(std::addressof(m_process_alias_size),    svc::InfoType_AliasRegionSize,    handle, 0));
            R_TRY(svc::GetInfo(std::addressof(m_process_heap_address),  svc::InfoType_HeapRegionAddress,  handle, 0));
            R_TRY(svc::GetInfo(std::addressof(m_process_heap_size),     svc::InfoType_HeapRegionSize,     handle, 0));
            R_TRY(svc::GetInfo(std::addressof(m_process_aslr_address),  svc::InfoType_AslrRegionAddress,  handle, 0));
            R_TRY(svc::GetInfo(std::addressof(m_process_aslr_size),     svc::InfoType_AslrRegionSize,     handle, 0));
            R_TRY(svc::GetInfo(std::addressof(m_process_stack_address), svc::InfoType_StackRegionAddress, handle, 0));
            R_TRY(svc::GetInfo(std::addressof(m_process_stack_size),    svc::InfoType_StackRegionSize,    handle, 0));

            if (m_program_location.program_id == ncm::InvalidProgramId) {
                R_TRY(svc::GetInfo(std::addressof(m_program_location.program_id.value), svc::InfoType_ProgramId, handle, 0));
            }

            u64 value;
            R_TRY(svc::GetInfo(std::addressof(value), svc::InfoType_IsApplication, handle, 0));
            m_is_application = value != 0;

            return ResultSuccess();
        };

        /* Get process info/status. */
        os::NativeHandle process_handle;
        if (R_FAILED(pm::dmnt::AtmosphereGetProcessInfo(std::addressof(process_handle), std::addressof(m_program_location), std::addressof(m_process_override_status), m_process_id))) {
            process_handle            = os::InvalidNativeHandle;
            m_program_location        = { ncm::InvalidProgramId, };
            m_process_override_status = {};
        }
        ON_SCOPE_EXIT { os::CloseNativeHandle(process_handle); };

        /* Try collecting from our debug handle, then the process handle. */
        if (R_FAILED(CollectProcessInfoImpl(m_debug_handle)) && R_FAILED(CollectProcessInfoImpl(process_handle))) {
            m_process_alias_address = 0;
            m_process_alias_size    = 0;
            m_process_heap_address  = 0;
            m_process_heap_size     = 0;
            m_process_aslr_address  = 0;
            m_process_aslr_size     = 0;
            m_process_stack_address = 0;
            m_process_stack_size    = 0;
            m_is_application        = false;
        }
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

    Result DebugProcess::QueryMemory(svc::MemoryInfo *out, uintptr_t address) {
        svc::PageInfo dummy;
        return svc::QueryDebugProcessMemory(out, std::addressof(dummy), m_debug_handle, address);
    }

    Result DebugProcess::Continue() {
        AMS_DMNT2_GDB_LOG_DEBUG("DebugProcess::Continue() all\n");

        u64 thread_ids[] = { 0 };
        R_TRY(svc::ContinueDebugEvent(m_debug_handle, svc::ContinueFlag_ExceptionHandled | svc::ContinueFlag_EnableExceptionEvent | svc::ContinueFlag_ContinueAll, thread_ids, util::size(thread_ids)));

        m_continue_thread_id = 0;
        m_status             = ProcessStatus_Running;

        this->SetLastThreadId(0);
        this->SetLastSignal(GdbSignal_Signal0);

        return ResultSuccess();
    }

    Result DebugProcess::Continue(u64 thread_id) {
        AMS_DMNT2_GDB_LOG_DEBUG("DebugProcess::Continue() thread_id=%lx\n", thread_id);

        u64 thread_ids[] = { thread_id };
        R_TRY(svc::ContinueDebugEvent(m_debug_handle, svc::ContinueFlag_ExceptionHandled | svc::ContinueFlag_EnableExceptionEvent, thread_ids, util::size(thread_ids)));

        m_continue_thread_id = thread_id;
        m_status             = ProcessStatus_Running;

        this->SetLastThreadId(0);
        this->SetLastSignal(GdbSignal_Signal0);

        return ResultSuccess();
    }

    Result DebugProcess::Step() {
        AMS_DMNT2_GDB_LOG_DEBUG("DebugProcess::Step() all\n");
        return this->Step(this->GetLastThreadId());
    }

    Result DebugProcess::Step(u64 thread_id) {
        AMS_DMNT2_GDB_LOG_DEBUG("DebugProcess::Step() thread_id=%lx\n", thread_id);

        /* Get the thread context. */
        svc::ThreadContext ctx;
        R_TRY(this->GetThreadContext(std::addressof(ctx), thread_id, svc::ThreadContextFlag_Control));

        /* Note that we're stepping. */
        m_stepping = true;

        if (m_use_hardware_single_step) {
            /* Set thread single step. */
            R_TRY(this->SetThreadContext(std::addressof(ctx), thread_id, svc::ThreadContextFlag_SetSingleStep));
        } else {
            /* Determine where we're stepping to. */
            u64 current_pc  = ctx.pc;
            u64 step_target = 0;
            this->GetBranchTarget(ctx, thread_id, current_pc, step_target);

            /* Ensure we end with valid breakpoints. */
            auto bp_guard = SCOPE_GUARD { this->ClearStep(); };

            /* Set step breakpoint on current pc. */
            /* TODO: aarch32 breakpoints. */
            if (current_pc) {
                R_TRY(m_step_breakpoints.SetBreakPoint(current_pc, sizeof(u32), true));
            }

            if (step_target) {
                R_TRY(m_step_breakpoints.SetBreakPoint(step_target, sizeof(u32), true));
            }

            bp_guard.Cancel();
        }

        return ResultSuccess();
    }

    void DebugProcess::ClearStep() {
        /* If we should, clear our step breakpoints. */
        if (m_stepping) {
            m_step_breakpoints.ClearStep();
            m_stepping = false;
        }
    }

    Result DebugProcess::Break() {
        if (this->GetStatus() == ProcessStatus_Running) {
            AMS_DMNT2_GDB_LOG_DEBUG("DebugProcess::Break\n");
            return svc::BreakDebugProcess(m_debug_handle);
        } else {
            AMS_DMNT2_GDB_LOG_ERROR("DebugProcess::Break called on non-running process!\n");
            return ResultSuccess();
        }
    }

    Result DebugProcess::Terminate() {
        if (this->IsValid()) {
            R_ABORT_UNLESS(svc::TerminateDebugProcess(m_debug_handle));
            this->Detach();
        }

        return ResultSuccess();
    }

    void DebugProcess::GetBranchTarget(svc::ThreadContext &ctx, u64 thread_id, u64 &current_pc, u64 &target) {
        /* Save pc, in case we modify it. */
        const u64 pc = current_pc;

        /* Clear the target. */
        target = 0;

        /* By default, we advance by four. */
        current_pc += 4;

        /* Get the instruction where we were. */
        u32 insn = 0;
        this->ReadMemory(std::addressof(insn), pc, sizeof(insn));

        /* Handle by architecture. */
        bool is_call = false;
        if (this->Is64Bit()) {
            if ((insn & 0x7C000000) == 0x14000000) {
                /* Unconditional branch (b/bl) */
                if (insn != 0x14000001) {
                    is_call    = (insn & 0x80000000) == 0x80000000;
                    current_pc = 0;
                    target     = SignExtend(((insn & 0x03FFFFFF) << 2), 28) + pc;
                }
            } else if ((insn & 0x7E000000) == 0x34000000) {
                /* Compare/Branch (cbz/cbnz) */
                target = SignExtend(((insn & 0x00FFFFE0) >> 3), 21) + pc;
            } else if ((insn & 0x7E000000) == 0x36000000) {
                /* Test and branch (tbz/tbnz) */
                target = SignExtend(((insn & 0x0007FFE0) >> 3), 16) + pc;
            } else if ((insn & 0xFF000010) == 0x54000000) {
                /* Conditional branch (b.*) */
                if ((insn & 0xF) == 0xE) {
                    /* Unconditional. */
                    current_pc = 0;
                }
                target = SignExtend(((insn & 0x00FFFFE0) >> 3), 21) + pc;
            } else if ((insn & 0xFF8FFC1F) == 0xD60F0000) {
                /* Unconditional branch */
                is_call = (insn & 0x00F00000) == 0x00300000;
                if (!is_call) {
                    current_pc = 0;
                }

                /* Get the register. */
                svc::ThreadContext new_ctx;
                if (R_SUCCEEDED(this->GetThreadContext(std::addressof(new_ctx), thread_id, svc::ThreadContextFlag_Control | svc::ThreadContextFlag_General))) {
                    const int reg = (insn & 0x03E0) >> 5;
                    if (reg < 29) {
                        target = new_ctx.r[reg];
                    } else if (reg == 29) {
                        target = new_ctx.fp;
                    } else if (reg == 30) {
                        target = new_ctx.lr;
                    } else if (reg == 31) {
                        target = new_ctx.sp;
                    }
                }
            }
        } else {
            /* TODO aarch32 branch decoding */
            AMS_UNUSED(ctx);
        }
    }

    Result DebugProcess::SetBreakPoint(uintptr_t address, size_t size, bool is_step) {
        return m_software_breakpoints.SetBreakPoint(address, size, is_step);
    }

    Result DebugProcess::ClearBreakPoint(uintptr_t address, size_t size) {
        m_software_breakpoints.ClearBreakPoint(address, size);
        return ResultSuccess();
    }

    Result DebugProcess::SetHardwareBreakPoint(uintptr_t address, size_t size, bool is_step) {
        return m_hardware_breakpoints.SetBreakPoint(address, size, is_step);
    }

    Result DebugProcess::ClearHardwareBreakPoint(uintptr_t address, size_t size) {
        m_hardware_breakpoints.ClearBreakPoint(address, size);
        return ResultSuccess();
    }

    Result DebugProcess::SetWatchPoint(u64 address, u64 size, bool read, bool write) {
        return m_hardware_watchpoints.SetWatchPoint(address, size, read, write);
    }

    Result DebugProcess::ClearWatchPoint(u64 address, u64 size) {
        return m_hardware_watchpoints.ClearBreakPoint(address, size);
    }

    Result DebugProcess::GetWatchPointInfo(u64 address, bool &read, bool &write) {
        return m_hardware_watchpoints.GetWatchPointInfo(address, read, write);
    }

    bool DebugProcess::IsValidWatchPoint(u64 address, u64 size) {
        return HardwareWatchPointManager::IsValidWatchPoint(address, size);
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

    void DebugProcess::GetThreadName(char *dst, u64 thread_id) const {
        for (size_t i = 0; i < ThreadCountMax; ++i) {
            if (m_thread_valid[i] && m_thread_ids[i] == thread_id) {
                if (R_FAILED(osdbg::GetThreadName(dst, std::addressof(m_thread_infos[i])))) {
                    if (m_thread_infos[i]._thread_type != 0) {
                        if (m_thread_infos[i]._thread_type_type == osdbg::ThreadTypeType_Libnx) {
                            util::TSNPrintf(dst, os::ThreadNameLengthMax, "libnx Thread_0x%010lx", reinterpret_cast<uintptr_t>(m_thread_infos[i]._thread_type));
                        } else {
                            util::TSNPrintf(dst, os::ThreadNameLengthMax, "Thread_0x%010lx", reinterpret_cast<uintptr_t>(m_thread_infos[i]._thread_type));
                        }
                    } else {
                        break;
                    }
                }
                return;
            }
        }

        util::TSNPrintf(dst, os::ThreadNameLengthMax, "Thread_ID=%lu", thread_id);
    }

}