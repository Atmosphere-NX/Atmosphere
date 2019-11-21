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
#include <stratosphere.hpp>
#include "amsmitm_initialization.hpp"
#include "amsmitm_fs_utils.hpp"

namespace ams::mitm {

    namespace {

        void InitializeThreadFunc(void *arg);

        constexpr size_t InitializeThreadStackSize = 0x4000;
        constexpr int    InitializeThreadPriority  = 0x15;
        os::StaticThread<InitializeThreadStackSize> g_initialize_thread(&InitializeThreadFunc, nullptr, InitializeThreadPriority);

        /* Globals. */
        os::Event g_init_event(false);

        /* Initialization implementation */
        void InitializeThreadFunc(void *arg) {
            /* Wait for the SD card to be ready. */
            cfg::WaitSdCardInitialized();

            /* TODO: Other initialization tasks. */

            /* Open global SD card file system, so that other threads can begin using the SD. */
            mitm::fs::OpenGlobalSdCardFileSystem();

            /* Signal to waiters that we are ready. */
            g_init_event.Signal();
        }

    }

    void StartInitialize() {
        R_ASSERT(g_initialize_thread.Start());
    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void WaitInitialized() {
        g_init_event.Wait();
    }

}
