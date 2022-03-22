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
#pragma once
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/htcs.hpp>
#include <stratosphere/scs/scs_command_processor.hpp>

namespace ams::scs {

    class ShellServer {
        private:
            htcs::HtcsPortName m_port_name;
            os::ThreadType m_thread;
            u8 m_buffer[64_KB];
            CommandProcessor *m_command_processor;
        private:
            static void ThreadEntry(void *arg) { reinterpret_cast<ShellServer *>(arg)->DoShellServer(); }

            void DoShellServer();
        public:
            constexpr ShellServer() = default;
        public:
            void Initialize(const char *port_name, void *stack, size_t stack_size, CommandProcessor *command_processor);
            void Start();
    };

}
