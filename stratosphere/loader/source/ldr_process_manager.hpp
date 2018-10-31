/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "ldr_registration.hpp"
#include "ldr_process_creation.hpp"

enum ProcessManagerServiceCmd {
    Pm_Cmd_CreateProcess = 0,
    Pm_Cmd_GetProgramInfo = 1,
    Pm_Cmd_RegisterTitle = 2,
    Pm_Cmd_UnregisterTitle = 3
};

class ProcessManagerService final : public IServiceObject {
    struct ProgramInfo {
        u8 main_thread_priority;
        u8 default_cpu_id;
        u16 application_type;
        u32 main_thread_stack_size;
        u64 title_id;
        u32 acid_sac_size;
        u32 aci0_sac_size;
        u32 acid_fac_size;
        u32 aci0_fah_size;
        u8 ac_buffer[0x3E0];
    };
    
    static_assert(sizeof(ProcessManagerService::ProgramInfo) == 0x400, "Incorrect ProgramInfo definition.");      
    private:
        /* Actual commands. */
        Result CreateProcess(Out<MovedHandle> proc_h, u64 index, u32 flags, CopiedHandle reslimit_h);
        Result GetProgramInfo(OutPointerWithServerSize<ProcessManagerService::ProgramInfo, 0x1> out_program_info, Registration::TidSid tid_sid);
        Result RegisterTitle(Out<u64> index, Registration::TidSid tid_sid);
        Result UnregisterTitle(u64 index);
        
        /* Utilities */
        Result PopulateProgramInfoBuffer(ProcessManagerService::ProgramInfo *out, Registration::TidSid *tid_sid);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Pm_Cmd_CreateProcess, &ProcessManagerService::CreateProcess>(),
            MakeServiceCommandMeta<Pm_Cmd_GetProgramInfo, &ProcessManagerService::GetProgramInfo>(),
            MakeServiceCommandMeta<Pm_Cmd_RegisterTitle, &ProcessManagerService::RegisterTitle>(),
            MakeServiceCommandMeta<Pm_Cmd_UnregisterTitle, &ProcessManagerService::UnregisterTitle>(),
        };
};
