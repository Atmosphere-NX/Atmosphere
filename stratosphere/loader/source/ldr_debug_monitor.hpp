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

enum DebugMonitorServiceCmd {
    Dmnt_Cmd_AddTitleToLaunchQueue = 0,
    Dmnt_Cmd_ClearLaunchQueue = 1,
    Dmnt_Cmd_GetNsoInfo = 2
};

class DebugMonitorService final : public IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override {
            /* This service will never defer. */
            return 0;
        }
        
        DebugMonitorService *clone() override {
            return new DebugMonitorService();
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result> add_title_to_launch_queue(u64 args_size, u64 tid, InPointer<char> args);
        std::tuple<Result> clear_launch_queue(u64 dat);
        std::tuple<Result, u32> get_nso_info(u64 pid, OutPointerWithClientSize<Registration::NsoInfo> out);
};
