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

namespace ams::scs {

    namespace {

        ServerManager g_server_manager;

    }

    ServerManager *GetServerManager() {
        return std::addressof(g_server_manager);
    }

    void StartServer() {
        /* Start the server. */
        g_server_manager.ResumeProcessing();

        /* Loop processing the server. */
        g_server_manager.LoopProcess();
    }

}
