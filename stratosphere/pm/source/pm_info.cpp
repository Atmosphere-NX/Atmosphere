/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stratosphere/ldr.hpp>
#include <stratosphere/ldr/ldr_pm_api.hpp>

#include "pm_registration.hpp"
#include "pm_info.hpp"

Result InformationService::GetTitleId(Out<u64> tid, u64 pid) {
    std::scoped_lock<ProcessList &> lk(Registration::GetProcessList());

    std::shared_ptr<Registration::Process> proc = Registration::GetProcess(pid);
    if (proc == NULL) {
        return ResultPmProcessNotFound;
    }

    tid.SetValue(proc->tid_sid.title_id);
    return ResultSuccess;
}

Result InformationService::AtmosphereGetProcessId(Out<u64> pid, u64 tid) {
    std::scoped_lock<ProcessList &> lk(Registration::GetProcessList());

    std::shared_ptr<Registration::Process> proc = Registration::GetProcessByTitleId(tid);
    if (proc == nullptr) {
        return ResultPmProcessNotFound;
    }

    pid.SetValue(proc->pid);
    return ResultSuccess;
}

Result InformationService::AtmosphereHasLaunchedTitle(Out<bool> out, u64 tid) {
    return sts::ldr::pm::HasLaunchedTitle(out.GetPointer(), sts::ncm::TitleId{tid});
}
