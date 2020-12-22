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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>

namespace ams::kern::KDumpObject {

    void DumpThread();
    void DumpThread(u64 thread_id);

    void DumpThreadCallStack();
    void DumpThreadCallStack(u64 thread_id);

    void DumpKernelObject();

    void DumpHandle();
    void DumpHandle(u64 process_id);

    void DumpKernelMemory();
    void DumpMemory();
    void DumpMemory(u64 process_id);

    void DumpKernelPageTable();
    void DumpPageTable();
    void DumpPageTable(u64 process_id);

    void DumpKernelCpuUtilization();
    void DumpCpuUtilization();
    void DumpCpuUtilization(u64 process_id);

    void DumpProcess();
    void DumpProcess(u64 process_id);

    void DumpPort();
    void DumpPort(u64 process_id);

}
