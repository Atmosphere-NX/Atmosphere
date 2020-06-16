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
#include "dmnt_service.hpp"

namespace ams::dmnt {

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

    Result DebugMonitorService::GetProcessId(sf::Out<os::ProcessId> out_pid, Handle hnd) {
        /* Nintendo discards the output of this command, but we will return it. */
        return os::TryGetProcessId(out_pid.GetPointer(), hnd);
    }

    Result DebugMonitorService::GetProcessHandle(sf::Out<Handle> out_hnd, os::ProcessId pid) {
        R_TRY_CATCH(svcDebugActiveProcess(out_hnd.GetPointer(), static_cast<u64>(pid))) {
            R_CONVERT(svc::ResultBusy, dbg::ResultAlreadyAttached());
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result DebugMonitorService::WaitSynchronization(Handle hnd, u64 ns) {
        /* Nintendo discards the output of this command, but we will return it. */
        return svcWaitSynchronizationSingle(hnd, ns);
    }

}
