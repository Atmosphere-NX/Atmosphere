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
#include <stratosphere/iserviceobject.hpp>

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
    
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override {
            /* This service will never defer. */
            return 0;
        }
        
        ProcessManagerService *clone() override {
            return new ProcessManagerService();
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result, MovedHandle> create_process(u64 flags, u64 index, CopiedHandle reslimit_h);
        std::tuple<Result> get_program_info(Registration::TidSid tid_sid, OutPointerWithServerSize<ProcessManagerService::ProgramInfo, 0x1> out_program_info);
        std::tuple<Result, u64> register_title(Registration::TidSid tid_sid);
        std::tuple<Result> unregister_title(u64 index);
        
        /* Utilities */
        Result populate_program_info_buffer(ProcessManagerService::ProgramInfo *out, Registration::TidSid *tid_sid);
};
