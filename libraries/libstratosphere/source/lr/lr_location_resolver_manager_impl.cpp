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
#include <stratosphere.hpp>
#include <stratosphere/lr/lr_location_resolver_manager_impl.hpp>
#include "lr_content_location_resolver_impl.hpp"
#include "lr_redirect_only_location_resolver_impl.hpp"
#include "lr_add_on_content_location_resolver_impl.hpp"
#include "lr_registered_location_resolver_impl.hpp"

namespace ams::lr {

    Result LocationResolverManagerImpl::OpenLocationResolver(sf::Out<std::shared_ptr<ILocationResolver>> out, ncm::StorageId storage_id) {
        std::scoped_lock lk(this->mutex);
        /* Find an existing resolver. */
        auto resolver = this->location_resolvers.Find(storage_id);

        /* No existing resolver is present, create one. */
        if (!resolver) {
            if (storage_id == ncm::StorageId::Host) {
                AMS_ABORT_UNLESS(this->location_resolvers.Insert(storage_id, sf::MakeShared<ILocationResolver, RedirectOnlyLocationResolverImpl>()));
            } else {
                auto content_resolver = sf::MakeShared<ILocationResolver, ContentLocationResolverImpl>(storage_id);
                R_TRY(content_resolver->GetImpl().Refresh());
                AMS_ABORT_UNLESS(this->location_resolvers.Insert(storage_id, std::move(content_resolver)));
            }

            /* Acquire the newly-created resolver. */
            resolver = this->location_resolvers.Find(storage_id);
        }

        /* Copy the output interface. */
        out.SetValue(std::shared_ptr<ILocationResolver>(*resolver));
        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::OpenRegisteredLocationResolver(sf::Out<std::shared_ptr<IRegisteredLocationResolver>> out) {
        std::scoped_lock lk(this->mutex);

        /* No existing resolver is present, create one. */
        if (!this->registered_location_resolver) {
            this->registered_location_resolver = sf::MakeShared<IRegisteredLocationResolver, RegisteredLocationResolverImpl>();
        }

        /* Copy the output interface. */
        out.SetValue(std::shared_ptr<IRegisteredLocationResolver>(this->registered_location_resolver));
        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::RefreshLocationResolver(ncm::StorageId storage_id) {
        std::scoped_lock lk(this->mutex);

        /* Attempt to find an existing resolver. */
        auto resolver = this->location_resolvers.Find(storage_id);
        R_UNLESS(resolver, lr::ResultUnknownStorageId());

        /* Refresh the resolver. */
        if (storage_id != ncm::StorageId::Host) {
            (*resolver)->Refresh();
        }

        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::OpenAddOnContentLocationResolver(sf::Out<std::shared_ptr<IAddOnContentLocationResolver>> out) {
        std::scoped_lock lk(this->mutex);

        /* No existing resolver is present, create one. */
        if (!this->add_on_content_location_resolver) {
            this->add_on_content_location_resolver = sf::MakeShared<IAddOnContentLocationResolver, AddOnContentLocationResolverImpl>();
        }

        /* Copy the output interface. */
        out.SetValue(std::shared_ptr<IAddOnContentLocationResolver>(this->add_on_content_location_resolver));
        return ResultSuccess();
    }

}
