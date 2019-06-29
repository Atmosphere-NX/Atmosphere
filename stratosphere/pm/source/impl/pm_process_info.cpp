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

#include <stratosphere/sm/sm_manager_api.hpp>
#include <stratosphere/ldr/ldr_pm_api.hpp>

#include "pm_process_info.hpp"

namespace sts::pm::impl {

    ProcessInfo::ProcessInfo(Handle h, u64 pid, ldr::PinId pin, const ncm::TitleLocation &l) : process_id(pid), pin_id(pin), loc(l), handle(h), state(ProcessState_Created), flags(0) {
        /* ... */
    }

    ProcessInfo::~ProcessInfo() {
        this->Cleanup();
    }

    void ProcessInfo::Cleanup() {
        if (this->handle != INVALID_HANDLE) {
            /* Unregister the process. */
            fsprUnregisterProgram(this->process_id);
            sm::manager::UnregisterProcess(this->process_id);
            ldr::pm::UnpinTitle(this->pin_id);

            /* Close the process's handle. */
            svcCloseHandle(this->handle);
            this->handle = INVALID_HANDLE;
        }
    }

}
