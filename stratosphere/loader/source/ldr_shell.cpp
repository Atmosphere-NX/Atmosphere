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

Result ShellService::AddTitleToLaunchQueue(u64 tid, InPointer<char> args, u32 args_size) {
    if (args.num_elements < args_size) args_size = args.num_elements;
    return LaunchQueue::Add(tid, args.pointer, args_size);
}

void ShellService::ClearLaunchQueue() {
    LaunchQueue::Clear();
}

/* SetExternalContentSource extension */
Result ShellService::SetExternalContentSource(Out<MovedHandle> out, u64 tid) {
    Handle server_h;
    Handle client_h;

    Result rc;
    if (R_FAILED(rc = svcCreateSession(&server_h, &client_h, 0, 0))) {
        return rc;
    }

    Service service;
    serviceCreate(&service, client_h);
    ContentManagement::SetExternalContentSource(tid, FsFileSystem {service});
    out.SetValue(server_h);
    return 0;
}
