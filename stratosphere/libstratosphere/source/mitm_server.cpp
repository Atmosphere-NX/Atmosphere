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

#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>

static HosMutex g_server_query_mutex;
static HosThread g_server_query_manager_thread;
static SessionManagerBase *g_server_query_manager = nullptr;

static void ServerQueryManagerThreadFunc(void *arg) {
    g_server_query_manager->Process();
}

void RegisterMitmServerQueryHandle(Handle query_h, ServiceObjectHolder &&service) {
    std::scoped_lock<HosMutex> lock(g_server_query_mutex);

    const bool exists = g_server_query_manager != nullptr;
    if (!exists) {
        /* Create a new waitable manager if it doesn't exist already. */
        static auto s_server_query_manager = WaitableManager(1);
        g_server_query_manager = &s_server_query_manager;
    }

    /* Add session to the manager. */
    g_server_query_manager->AddSession(query_h, std::move(service));

    /* If this is our first time, launch thread. */
    if (!exists) {
        R_ASSERT(g_server_query_manager_thread.Initialize(&ServerQueryManagerThreadFunc, nullptr, 0x4000, 27));
        R_ASSERT(g_server_query_manager_thread.Start());
    }
}