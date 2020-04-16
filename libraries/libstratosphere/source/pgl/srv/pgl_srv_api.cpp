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
#include "pgl_srv_shell.hpp"

namespace ams::pgl::srv {

    void Initialize(ShellInterface *interface, MemoryResource *mr) {
        /* Set the memory resource for the interface. */
        interface->Initialize(mr);

        /* Enable extra application threads, if we should. */
        u8 enable_application_extra_thread;
        const size_t sz = settings::fwdbg::GetSettingsItemValue(std::addressof(enable_application_extra_thread), sizeof(enable_application_extra_thread), "application_extra_thread", "enable_application_extra_thread");
        if (sz == sizeof(enable_application_extra_thread) && enable_application_extra_thread != 0) {
            /* NOTE: Nintendo does not check that this succeeds. */
            pm::shell::EnableApplicationExtraThread();
        }

        /* Start the Process Tracking thread. */
        pgl::srv::InitializeProcessControlTask();
    }


}