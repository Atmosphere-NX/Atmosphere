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
#include "lr_content_location_resolver_impl.hpp"
#include "lr_redirect_only_location_resolver_impl.hpp"
#include "lr_add_on_content_location_resolver_impl.hpp"
#include "lr_registered_location_resolver_impl.hpp"

namespace ams::lr {

    namespace {

        using ContentLocationResolverFactory      = sf::ObjectFactory<sf::StdAllocationPolicy<std::allocator>>;
        using RedirectOnlyLocationResolverFactory = sf::ObjectFactory<sf::StdAllocationPolicy<std::allocator>>;

    }

    Result LocationResolverManagerImpl::OpenLocationResolver(sf::Out<sf::SharedPointer<ILocationResolver>> out, ncm::StorageId storage_id) {
        std::scoped_lock lk(m_mutex);
        /* Find an existing resolver. */
        auto resolver = m_location_resolvers.Find(storage_id);

        /* No existing resolver is present, create one. */
        if (!resolver) {
            if (storage_id == ncm::StorageId::Host) {
                AMS_ABORT_UNLESS(m_location_resolvers.Insert(storage_id, RedirectOnlyLocationResolverFactory::CreateSharedEmplaced<ILocationResolver, RedirectOnlyLocationResolverImpl>()));
            } else {
                auto content_resolver = ContentLocationResolverFactory::CreateSharedEmplaced<ILocationResolver, ContentLocationResolverImpl>(storage_id);
                R_TRY(content_resolver->Refresh());
                AMS_ABORT_UNLESS(m_location_resolvers.Insert(storage_id, std::move(content_resolver)));
            }

            /* Acquire the newly-created resolver. */
            resolver = m_location_resolvers.Find(storage_id);
        }

        /* Copy the output interface. */
        *out = *resolver;
        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::OpenRegisteredLocationResolver(sf::Out<sf::SharedPointer<IRegisteredLocationResolver>> out) {
        std::scoped_lock lk(m_mutex);

        /* No existing resolver is present, create one. */
        if (!m_registered_location_resolver) {
            m_registered_location_resolver = ContentLocationResolverFactory::CreateSharedEmplaced<IRegisteredLocationResolver, RegisteredLocationResolverImpl>();
        }

        /* Copy the output interface. */
        *out = m_registered_location_resolver;
        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::RefreshLocationResolver(ncm::StorageId storage_id) {
        std::scoped_lock lk(m_mutex);

        /* Attempt to find an existing resolver. */
        auto resolver = m_location_resolvers.Find(storage_id);
        R_UNLESS(resolver, lr::ResultUnknownStorageId());

        /* Refresh the resolver. */
        if (storage_id != ncm::StorageId::Host) {
            (*resolver)->Refresh();
        }

        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::OpenAddOnContentLocationResolver(sf::Out<sf::SharedPointer<IAddOnContentLocationResolver>> out) {
        std::scoped_lock lk(m_mutex);

        /* No existing resolver is present, create one. */
        if (!m_add_on_content_location_resolver) {
            m_add_on_content_location_resolver = ContentLocationResolverFactory::CreateSharedEmplaced<IAddOnContentLocationResolver, AddOnContentLocationResolverImpl>();
        }

        /* Copy the output interface. */
        *out = m_add_on_content_location_resolver;
        return ResultSuccess();
    }

}
