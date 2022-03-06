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
#include <stratosphere/lr/lr_location_resolver_manager_impl.hpp>
#include "lr_remote_location_resolver_manager_impl.hpp"

namespace ams::lr {

    namespace {

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    LocationResolverManagerAllocator::Initialize(lmem::CreateOption_None);
                }
        } g_static_allocator_initializer;

    }

    sf::SharedPointer<ILocationResolverManager> GetLocationResolverManagerService() {
        #if defined(ATMOSPHERE_OS_HORIZON)
        return LocationResolverManagerFactory::CreateSharedEmplaced<ILocationResolverManager, RemoteLocationResolverManagerImpl>();
        #else
        return LocationResolverManagerFactory::CreateSharedEmplaced<ILocationResolverManager, LocationResolverManagerImpl>();
        #endif
    }
}
