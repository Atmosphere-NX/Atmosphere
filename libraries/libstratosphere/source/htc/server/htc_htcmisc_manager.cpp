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
#include "htc_htcmisc_manager.hpp"

namespace ams::htc::server {

    namespace {


        mem::StandardAllocator g_allocator;
        sf::StandardAllocatorMemoryResource g_sf_resource(std::addressof(g_allocator));

        alignas(os::MemoryPageSize) constinit u8 g_heap[util::AlignUp(sizeof(HtcServiceObject), os::MemoryPageSize) + 12_KB];

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    g_allocator.Initialize(g_heap, sizeof(g_heap));
                }
        } g_static_allocator_initializer;

        using ObjectFactory = sf::ObjectFactory<sf::MemoryResourceAllocationPolicy>;

    }

    sf::SharedPointer<tma::IHtcManager> CreateHtcmiscManager(HtcServiceObject **out, htclow::HtclowManager *htclow_manager) {
        auto obj = ObjectFactory::CreateSharedEmplaced<tma::IHtcManager, HtcServiceObject>(std::addressof(g_sf_resource), htclow_manager);
        *out = std::addressof(obj.GetImpl());
        return obj;
    }

}
