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

namespace ams::lr {

    Result LocationResolverManagerImpl::OpenLocationResolver(sf::Out<std::shared_ptr<ILocationResolverInterface>> out, ncm::StorageId storage_id) {
        std::scoped_lock lk(this->mutex);
        /* Find an existing resolver. */
        auto resolver = this->location_resolvers.Find(storage_id);

        /* No existing resolver is present, create one. */
        if (!resolver) {
            if (storage_id == ncm::StorageId::Host) {
                this->location_resolvers[storage_id] = std::make_shared<RedirectOnlyLocationResolverInterface>();
            } else {
                auto content_resolver = std::make_shared<ContentLocationResolverInterface>(storage_id);
                R_TRY(content_resolver->Refresh());
                this->location_resolvers[storage_id] = std::move(content_resolver);
            }

            /* Acquire the newly-created resolver. */
            resolver = this->location_resolvers.Find(storage_id);
        }

        /* Copy the output interface. */
        std::shared_ptr<ILocationResolverInterface> new_intf = *resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::OpenRegisteredLocationResolver(sf::Out<std::shared_ptr<RegisteredLocationResolverInterface>> out) {
        std::scoped_lock lk(this->mutex);

        /* No existing resolver is present, create one. */
        if (!this->registered_location_resolver) {
            this->registered_location_resolver = std::make_shared<RegisteredLocationResolverInterface>();
        }

        /* Copy the output interface. */
        std::shared_ptr<RegisteredLocationResolverInterface> new_intf = this->registered_location_resolver;
        out.SetValue(std::move(new_intf));
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

    Result LocationResolverManagerImpl::OpenAddOnContentLocationResolver(sf::Out<std::shared_ptr<AddOnContentLocationResolverInterface>> out) {
        std::scoped_lock lk(this->mutex);

        /* No existing resolver is present, create one. */
        if (!this->add_on_content_location_resolver) {
            this->add_on_content_location_resolver = std::make_shared<AddOnContentLocationResolverInterface>();
        }

        /* Copy the output interface. */
        std::shared_ptr<AddOnContentLocationResolverInterface> new_intf = this->add_on_content_location_resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }

}
