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
#include "dmnt_service.hpp"

Result DebugMonitorService::BreakDebugProcess(Handle debug_hnd) {
    /* Nintendo discards the output of this command, but we will return it. */
    return svcBreakDebugProcess(debug_hnd);
}

Result DebugMonitorService::TerminateDebugProcess(Handle debug_hnd) {
    /* Nintendo discards the output of this command, but we will return it. */
    return svcTerminateDebugProcess(debug_hnd);
}

Result DebugMonitorService::CloseHandle(Handle debug_hnd) {
    /* Nintendo discards the output of this command, but we will return it. */
    /* This command is, entertainingly, also pretty unsafe in general... */
    return svcCloseHandle(debug_hnd);
}

Result DebugMonitorService::GetProcessId(Out<u64> out_pid, Handle hnd) {
    /* Nintendo discards the output of this command, but we will return it. */
    return svcGetProcessId(out_pid.GetPointer(), hnd);
}

Result DebugMonitorService::GetProcessHandle(Out<Handle> out_hnd, u64 pid) {
    Result rc = svcDebugActiveProcess(out_hnd.GetPointer(), pid);
    if (rc == ResultKernelAlreadyExists) {
        rc = ResultDebugAlreadyAttached;
    }
    return rc;
}

Result DebugMonitorService::WaitSynchronization(Handle hnd, u64 ns) {
    /* Nintendo discards the output of this command, but we will return it. */
    return svcWaitSynchronizationSingle(hnd, ns);
}
