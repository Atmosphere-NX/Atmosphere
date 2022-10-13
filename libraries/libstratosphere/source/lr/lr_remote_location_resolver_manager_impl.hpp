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
#pragma once
#include <stratosphere.hpp>
#include "lr_location_resolver_manager_factory.hpp"
#include "lr_remote_location_resolver_impl.hpp"
#include "lr_remote_registered_location_resolver_impl.hpp"

namespace ams::lr {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteLocationResolverManagerImpl {
        public:
            RemoteLocationResolverManagerImpl() { R_ABORT_UNLESS(::lrInitialize()); }

            ~RemoteLocationResolverManagerImpl() { ::lrExit(); }
        public:
            /* Actual commands. */
            Result OpenLocationResolver(sf::Out<sf::SharedPointer<ILocationResolver>> out, ncm::StorageId storage_id) {
                LrLocationResolver lr;
                R_TRY(::lrOpenLocationResolver(static_cast<::NcmStorageId>(storage_id), std::addressof(lr)));

                *out = LocationResolverManagerFactory::CreateSharedEmplaced<ILocationResolver, RemoteLocationResolverImpl>(lr);
                R_SUCCEED();
            }

            Result OpenRegisteredLocationResolver(sf::Out<sf::SharedPointer<IRegisteredLocationResolver>> out) {
                LrRegisteredLocationResolver lr;
                R_TRY(::lrOpenRegisteredLocationResolver(std::addressof(lr)));

                *out = LocationResolverManagerFactory::CreateSharedEmplaced<IRegisteredLocationResolver, RemoteRegisteredLocationResolverImpl>(lr);
                R_SUCCEED();
            }

            Result RefreshLocationResolver(ncm::StorageId storage_id) {
                AMS_UNUSED(storage_id);
                AMS_ABORT("TODO: libnx binding");
            }

            Result OpenAddOnContentLocationResolver(sf::Out<sf::SharedPointer<IAddOnContentLocationResolver>> out) {
                AMS_UNUSED(out);
                AMS_ABORT("TODO: libnx binding");
            }

            Result SetEnabled(const sf::InMapAliasArray<ncm::StorageId> &storages) {
                AMS_UNUSED(storages);
                AMS_ABORT("TODO: libnx binding");
            }
    };
    static_assert(lr::IsILocationResolverManager<RemoteLocationResolverManagerImpl>);
    #endif

}
