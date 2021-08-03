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

namespace ams::scs {

    void ShellServer::Initialize(const char *port_name, void *stack, size_t stack_size, CommandProcessor *command_processor) {
        /* Set our variables. */
        m_command_processor = command_processor;
        std::strcpy(m_port_name.name, port_name);

        /* Create our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_thread), ThreadEntry, this, stack, stack_size, AMS_GET_SYSTEM_THREAD_PRIORITY(scs, ShellServer)));

        /* Set our thread's name. */
        os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(scs, ShellServer));
    }

    void ShellServer::Start() {
        os::StartThread(std::addressof(m_thread));
    }

    void ShellServer::DoShellServer() {
        /* TODO */
        AMS_ABORT("ShellServer::DoShellServer");
    }

}
