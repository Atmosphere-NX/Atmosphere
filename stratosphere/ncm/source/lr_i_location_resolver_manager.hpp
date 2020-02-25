/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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

#include "lr_add_on_content_location_resolver.hpp"
#include "lr_i_location_resolver.hpp"
#include "lr_registered_location_resolver.hpp"

namespace ams::lr {

    class ILocationResolverManager : public sf::IServiceObject {
        protected:
            enum class CommandId {
                OpenLocationResolver                = 0,
                OpenRegisteredLocationResolver      = 1,
                RefreshLocationResolver             = 2,
                OpenAddOnContentLocationResolver    = 3,
            };
        public:
            /* Actual commands. */
            virtual Result OpenLocationResolver(sf::Out<std::shared_ptr<ILocationResolver>> out, ncm::StorageId storage_id) = 0;
            virtual Result OpenRegisteredLocationResolver(sf::Out<std::shared_ptr<RegisteredLocationResolverInterface>> out) = 0;
            virtual Result RefreshLocationResolver(ncm::StorageId storage_id) = 0;
            virtual Result OpenAddOnContentLocationResolver(sf::Out<std::shared_ptr<AddOnContentLocationResolverInterface>> out) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(OpenLocationResolver),
                MAKE_SERVICE_COMMAND_META(OpenRegisteredLocationResolver),
                MAKE_SERVICE_COMMAND_META(RefreshLocationResolver),
                MAKE_SERVICE_COMMAND_META(OpenAddOnContentLocationResolver, hos::Version_200),
            };
    };

}
