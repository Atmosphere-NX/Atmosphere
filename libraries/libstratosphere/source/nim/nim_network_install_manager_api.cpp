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

namespace ams::nim {

    namespace {

        bool g_initialized;

    }
    /* Management. */
    void InitializeForNetworkInstallManager() {
        AMS_ASSERT(!g_initialized);
        R_ABORT_UNLESS(nimInitialize());
        g_initialized = true;
    }

    void FinalizeForNetworkInstallManager() {
        AMS_ASSERT(g_initialized);
        nimExit();
        g_initialized = false;
    }

    /* Service API. */
    Result DestroySystemUpdateTask(const SystemUpdateTaskId &id) {
        static_assert(sizeof(SystemUpdateTaskId) == sizeof(::NimSystemUpdateTaskId));
        return nimDestroySystemUpdateTask(reinterpret_cast<const ::NimSystemUpdateTaskId *>(std::addressof(id)));
    }

    s32 ListSystemUpdateTask(SystemUpdateTaskId *out_list, size_t out_list_size) {
        static_assert(sizeof(SystemUpdateTaskId) == sizeof(::NimSystemUpdateTaskId));

        s32 count;
        R_ABORT_UNLESS(nimListSystemUpdateTask(std::addressof(count), reinterpret_cast<::NimSystemUpdateTaskId *>(out_list), out_list_size));

        return count;
    }

}
