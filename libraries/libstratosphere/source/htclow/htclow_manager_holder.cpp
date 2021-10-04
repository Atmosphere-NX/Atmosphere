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
#include "htclow_manager.hpp"

namespace ams::htclow::HtclowManagerHolder {

    namespace {

        constinit os::SdkMutex g_holder_mutex;
        constinit int g_holder_reference_count = 0;

        mem::StandardAllocator g_allocator;

        constinit HtclowManager *g_manager = nullptr;

        constinit htclow::impl::DriverType g_default_driver_type = htclow::impl::DriverType::Socket;

        alignas(os::MemoryPageSize) u8 g_heap_buffer[928_KB];

    }

    void AddReference() {
        std::scoped_lock lk(g_holder_mutex);

        if ((g_holder_reference_count++) == 0) {
            /* Initialize the allocator for the manager. */
            g_allocator.Initialize(g_heap_buffer, sizeof(g_heap_buffer));

            /* Allocate the manager. */
            g_manager = static_cast<HtclowManager *>(g_allocator.Allocate(sizeof(HtclowManager), alignof(HtclowManager)));

            /* Construct the manager. */
            std::construct_at(g_manager, std::addressof(g_allocator));

            /* Open the driver. */
            R_ABORT_UNLESS(g_manager->OpenDriver(g_default_driver_type));
        }

        AMS_ASSERT(g_holder_reference_count > 0);
    }

    void Release() {
        std::scoped_lock lk(g_holder_mutex);

        AMS_ASSERT(g_holder_reference_count > 0);

        if ((--g_holder_reference_count) == 0) {
            /* Disconnect. */
            g_manager->Disconnect();

            /* Close the driver. */
            g_manager->CloseDriver();

            /* Destroy the manager. */
            std::destroy_at(g_manager);
            g_allocator.Free(g_manager);
            g_manager = nullptr;

            /* Finalize the allocator. */
            g_allocator.Finalize();
        }
    }

    HtclowManager *GetHtclowManager() {
        std::scoped_lock lk(g_holder_mutex);

        return g_manager;
    }

    void SetDefaultDriver(htclow::impl::DriverType driver_type) {
        g_default_driver_type = driver_type;
    }

}
