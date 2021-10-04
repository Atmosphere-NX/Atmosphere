/*
 * Copyright (c) Atmosphère-NX
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
#include "pm_process_info.hpp"

namespace ams::pm::impl {

    ProcessInfo::ProcessInfo(os::NativeHandle h, os::ProcessId pid, ldr::PinId pin, const ncm::ProgramLocation &l, const cfg::OverrideStatus &s) : process_id(pid), pin_id(pin), loc(l), status(s), handle(h), state(svc::ProcessState_Created), flags(0) {
        os::InitializeMultiWaitHolder(std::addressof(this->multi_wait_holder), this->handle);
        os::SetMultiWaitHolderUserData(std::addressof(this->multi_wait_holder), reinterpret_cast<uintptr_t>(this));
    }

    ProcessInfo::~ProcessInfo() {
        this->Cleanup();
    }

    void ProcessInfo::Cleanup() {
        if (this->handle != os::InvalidNativeHandle) {
            /* Unregister the process. */
            fsprUnregisterProgram(static_cast<u64>(this->process_id));
            sm::manager::UnregisterProcess(this->process_id);
            ldr::pm::UnpinProgram(this->pin_id);

            /* Close the process's handle. */
            os::CloseNativeHandle(this->handle);
            this->handle = os::InvalidNativeHandle;

            /* Unlink the process from its multi wait. */
            os::UnlinkMultiWaitHolder(std::addressof(this->multi_wait_holder));
        }
    }

}
