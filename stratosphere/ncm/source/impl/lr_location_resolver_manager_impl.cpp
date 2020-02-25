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

#include "../lr_content_location_resolver.hpp"
#include "../lr_redirect_only_location_resolver.hpp"
#include "lr_location_resolver_manager_impl.hpp"

namespace ams::lr::impl {

    Result LocationResolverManagerImpl::OpenLocationResolver(sf::Out<std::shared_ptr<ILocationResolverInterface>> out, ncm::StorageId storage_id) {
        std::scoped_lock lk(g_mutex);
        auto resolver = g_location_resolvers.Find(storage_id);

        if (!resolver) {
            if (storage_id == ncm::StorageId::Host) {
                g_location_resolvers[storage_id] = std::make_shared<RedirectOnlyLocationResolverInterface>();
            } else {
                auto content_resolver = std::make_shared<ContentLocationResolverInterface>(storage_id);
                R_TRY(content_resolver->Refresh());
                g_location_resolvers[storage_id] = std::move(content_resolver);
            }
            resolver = g_location_resolvers.Find(storage_id);
        }

        std::shared_ptr<ILocationResolverInterface> new_intf = *resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::OpenRegisteredLocationResolver(sf::Out<std::shared_ptr<RegisteredLocationResolverInterface>> out) {
        std::scoped_lock lk(g_mutex);

        if (!g_registered_location_resolver) {
            g_registered_location_resolver = std::make_shared<RegisteredLocationResolverInterface>();
        }

        std::shared_ptr<RegisteredLocationResolverInterface> new_intf = g_registered_location_resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }
    
    Result LocationResolverManagerImpl::RefreshLocationResolver(ncm::StorageId storage_id) {
        std::scoped_lock lk(g_mutex);
        auto resolver = g_location_resolvers.Find(storage_id);

        R_UNLESS(resolver, lr::ResultUnknownStorageId());

        if (storage_id != ncm::StorageId::Host) {
            (*resolver)->Refresh();
        }

        return ResultSuccess();
    }

    Result LocationResolverManagerImpl::OpenAddOnContentLocationResolver(sf::Out<std::shared_ptr<AddOnContentLocationResolverInterface>> out) {
        std::scoped_lock lk(g_mutex);

        if (!g_add_on_content_location_resolver) {
            g_add_on_content_location_resolver = std::make_shared<AddOnContentLocationResolverInterface>();
        }
        
        std::shared_ptr<AddOnContentLocationResolverInterface> new_intf = g_add_on_content_location_resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }

}
