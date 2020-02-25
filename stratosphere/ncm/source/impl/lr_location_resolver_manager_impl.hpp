/*
 * Copyright (c) 2019 Adubbz
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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "../lr_addoncontentlocationresolver.hpp"
#include "../lr_ilocationresolver.hpp"
#include "../lr_i_location_resolver_manager.hpp"
#include "../lr_registeredlocationresolver.hpp"
#include "ncm_bounded_map.hpp"

namespace ams::lr::impl {

    class LocationResolverManagerImpl final : public ILocationResolverManager {
        private:
            ncm::impl::BoundedMap<ncm::StorageId, std::shared_ptr<ILocationResolver>, 5> g_location_resolvers;
            std::shared_ptr<RegisteredLocationResolverInterface> g_registered_location_resolver = nullptr;
            std::shared_ptr<AddOnContentLocationResolverInterface> g_add_on_content_location_resolver = nullptr;
            os::Mutex g_mutex;
        public:
            /* Actual commands. */
            virtual Result OpenLocationResolver(sf::Out<std::shared_ptr<ILocationResolver>> out, ncm::StorageId storage_id) override;
            virtual Result OpenRegisteredLocationResolver(sf::Out<std::shared_ptr<RegisteredLocationResolverInterface>> out) override;
            virtual Result RefreshLocationResolver(ncm::StorageId storage_id) override;
            virtual Result OpenAddOnContentLocationResolver(sf::Out<std::shared_ptr<AddOnContentLocationResolverInterface>> out) override;
    };

}
