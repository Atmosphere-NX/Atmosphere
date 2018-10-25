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
 
#include <switch.h>
#include <stratosphere.hpp>
#include "ldr_shell.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_content_management.hpp"

Result ShellService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    
    Result rc = 0xF601;
        
    switch ((ShellServiceCmd)cmd_id) {
        case Shell_Cmd_AddTitleToLaunchQueue:
            rc = WrapIpcCommandImpl<&ShellService::add_title_to_launch_queue>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Shell_Cmd_ClearLaunchQueue:
            rc = WrapIpcCommandImpl<&ShellService::clear_launch_queue>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Shell_Cmd_AtmosphereSetExternalContentSource:
            rc = WrapIpcCommandImpl<&ShellService::set_external_content_source>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

std::tuple<Result> ShellService::add_title_to_launch_queue(u64 args_size, u64 tid, InPointer<char> args) {
    fprintf(stderr, "Add to launch queue: %p, %zX\n", args.pointer, std::min(args_size, args.num_elements));
    return {LaunchQueue::add(tid, args.pointer, std::min(args_size, args.num_elements))};
}

std::tuple<Result> ShellService::clear_launch_queue(u64 dat) {
    fprintf(stderr, "Clear launch queue: %lx\n", dat);
    LaunchQueue::clear();
    return {0};
}

/* SetExternalContentSource extension */
std::tuple<Result, MovedHandle> ShellService::set_external_content_source(u64 tid) {
    Handle server_h;
    Handle client_h;

    Result rc;
    if (R_FAILED(rc = svcCreateSession(&server_h, &client_h, 0, 0))) {
        return {rc, 0};
    }

    Service service;
    serviceCreate(&service, client_h);
    ContentManagement::SetExternalContentSource(tid, FsFileSystem {service});
    return {0, server_h};
}
