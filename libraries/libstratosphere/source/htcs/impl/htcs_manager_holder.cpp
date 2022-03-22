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
#include "htcs_manager.hpp"

namespace ams::htcs::impl::HtcsManagerHolder {

    namespace {

        constinit os::SdkMutex g_holder_mutex;
        constinit int g_holder_reference_count = 0;

        mem::StandardAllocator g_allocator;

        constinit HtcsManager *g_manager = nullptr;

        alignas(os::MemoryPageSize) u8 g_heap_buffer[416_KB];

    }

    void AddReference() {
        std::scoped_lock lk(g_holder_mutex);

        if ((g_holder_reference_count++) == 0) {
            /* Add reference to the htclow manager. */
            htclow::HtclowManagerHolder::AddReference();

            /* Initialize the allocator for the manager. */
            g_allocator.Initialize(g_heap_buffer, sizeof(g_heap_buffer));

            /* Allocate the manager. */
            g_manager = static_cast<HtcsManager *>(g_allocator.Allocate(sizeof(HtcsManager), alignof(HtcsManager)));

            /* Construct the manager. */
            std::construct_at(g_manager, std::addressof(g_allocator), htclow::HtclowManagerHolder::GetHtclowManager());
        }

        AMS_ASSERT(g_holder_reference_count > 0);
    }

    void Release() {
        std::scoped_lock lk(g_holder_mutex);

        AMS_ASSERT(g_holder_reference_count > 0);

        if ((--g_holder_reference_count) == 0) {
            /* Destroy the manager. */
            std::destroy_at(g_manager);
            g_allocator.Free(g_manager);
            g_manager = nullptr;

            /* Finalize the allocator. */
            g_allocator.Finalize();

            /* Release reference to the htclow manager. */
            htclow::HtclowManagerHolder::Release();
        }
    }

    HtcsManager *GetHtcsManager() {
        std::scoped_lock lk(g_holder_mutex);

        return g_manager;
    }

}
