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
#include "lr_location_resolver_manager_factory.hpp"

namespace ams::lr {

    namespace {

        constinit sf::SharedPointer<ILocationResolverManager> g_location_resolver_manager;

    }

    void Initialize() {
        AMS_ASSERT(g_location_resolver_manager == nullptr);
        g_location_resolver_manager = GetLocationResolverManagerService();
    }

    void Finalize() {
        AMS_ASSERT(g_location_resolver_manager != nullptr);
        g_location_resolver_manager = nullptr;
    }


    Result OpenLocationResolver(LocationResolver *out, ncm::StorageId storage_id) {
        sf::SharedPointer<lr::ILocationResolver> lr;
        R_TRY(g_location_resolver_manager->OpenLocationResolver(std::addressof(lr), storage_id));

        *out = LocationResolver(std::move(lr));
        R_SUCCEED();
    }

    Result OpenRegisteredLocationResolver(RegisteredLocationResolver *out) {
        sf::SharedPointer<lr::IRegisteredLocationResolver> lr;
        R_TRY(g_location_resolver_manager->OpenRegisteredLocationResolver(std::addressof(lr)));

        *out = RegisteredLocationResolver(std::move(lr));
        R_SUCCEED();
    }

    Result OpenAddOnContentLocationResolver(AddOnContentLocationResolver *out) {
        sf::SharedPointer<lr::IAddOnContentLocationResolver> lr;
        R_TRY(g_location_resolver_manager->OpenAddOnContentLocationResolver(std::addressof(lr)));

        *out = AddOnContentLocationResolver(std::move(lr));
        R_SUCCEED();
    }

    Result RefreshLocationResolver(ncm::StorageId storage_id) {
        R_RETURN(g_location_resolver_manager->RefreshLocationResolver(storage_id));
    }
}
