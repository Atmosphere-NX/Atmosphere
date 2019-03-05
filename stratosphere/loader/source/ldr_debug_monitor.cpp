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
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>
#include "ldr_debug_monitor.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_registration.hpp"

Result DebugMonitorService::AddTitleToLaunchQueue(u64 tid, InPointer<char> args, u32 args_size) {
    if (args.num_elements < args_size) args_size = args.num_elements;
    return LaunchQueue::Add(tid, args.pointer, args_size);
}

void DebugMonitorService::ClearLaunchQueue() {
    LaunchQueue::Clear();
}

Result DebugMonitorService::GetNsoInfo(Out<u32> count, OutPointerWithClientSize<Registration::NsoInfo> out, u64 pid) {
    /* Zero out the output memory. */
    std::fill(out.pointer, out.pointer + out.num_elements, (const Registration::NsoInfo){0});
    /* Actually return the nso infos. */
    return Registration::GetNsoInfosForProcessId(out.pointer, out.num_elements, pid, count.GetPointer());
}
