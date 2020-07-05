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
#include <stratosphere/lr/lr_types.hpp>
#include <stratosphere/lr/lr_i_location_resolver.hpp>
#include <stratosphere/lr/lr_i_add_on_content_location_resolver.hpp>
#include <stratosphere/lr/lr_i_registered_location_resolver.hpp>

namespace ams::lr {

    #define AMS_LR_I_LOCATION_RESOLVER_MANAGER_INTERFACE_INFO(C, H)                                                                                                             \
        AMS_SF_METHOD_INFO(C, H, 0, Result, OpenLocationResolver,             (sf::Out<std::shared_ptr<ILocationResolver>> out, ncm::StorageId storage_id))                     \
        AMS_SF_METHOD_INFO(C, H, 1, Result, OpenRegisteredLocationResolver,   (sf::Out<std::shared_ptr<IRegisteredLocationResolver>> out))                                      \
        AMS_SF_METHOD_INFO(C, H, 2, Result, RefreshLocationResolver,          (ncm::StorageId storage_id))                                                                      \
        AMS_SF_METHOD_INFO(C, H, 3, Result, OpenAddOnContentLocationResolver, (sf::Out<std::shared_ptr<IAddOnContentLocationResolver>> out),                hos::Version_2_0_0)

    AMS_SF_DEFINE_INTERFACE(ILocationResolverManager, AMS_LR_I_LOCATION_RESOLVER_MANAGER_INTERFACE_INFO)

}
