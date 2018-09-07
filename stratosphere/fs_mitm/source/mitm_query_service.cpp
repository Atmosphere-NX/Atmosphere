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
 
#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>
#include "mitm_query_service.hpp"

static std::vector<u64> g_known_pids;
static std::vector<u64> g_known_tids;
static HosMutex g_pid_tid_mutex;

Result MitMQueryUtils::get_associated_tid_for_pid(u64 pid, u64 *tid) {
    Result rc = 0xCAFE;
    std::scoped_lock lk{g_pid_tid_mutex};
    for (unsigned int i = 0; i < g_known_pids.size(); i++) {
        if (g_known_pids[i] == pid) {
            *tid = g_known_tids[i];
            rc = 0x0;
            break;
        }
    }
    return rc;
}

void MitMQueryUtils::associate_pid_to_tid(u64 pid, u64 tid) {
    std::scoped_lock lk{g_pid_tid_mutex};
    g_known_pids.push_back(pid);
    g_known_tids.push_back(tid);
}
