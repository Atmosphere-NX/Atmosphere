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

#include "../lr_contentlocationresolver.hpp"
#include "../lr_redirectonlylocationresolver.hpp"
#include "lr_manager.hpp"
#include "ncm_bounded_map.hpp"

namespace ams::lr::impl {

    namespace {

        ncm::impl::BoundedMap<ncm::StorageId, std::shared_ptr<ILocationResolver>, 5> g_location_resolvers;
        std::shared_ptr<RegisteredLocationResolverInterface> g_registered_location_resolver = nullptr;
        std::shared_ptr<AddOnContentLocationResolverInterface> g_add_on_content_location_resolver = nullptr;
        os::Mutex g_mutex;

    }

    Result OpenLocationResolver(sf::Out<std::shared_ptr<ILocationResolver>> out, ncm::StorageId storage_id) {
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

        std::shared_ptr<ILocationResolver> new_intf = *resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }

    Result OpenRegisteredLocationResolver(sf::Out<std::shared_ptr<RegisteredLocationResolverInterface>> out) {
        std::scoped_lock lk(g_mutex);

        if (!g_registered_location_resolver) {
            g_registered_location_resolver = std::make_shared<RegisteredLocationResolverInterface>();
        }

        std::shared_ptr<RegisteredLocationResolverInterface> new_intf = g_registered_location_resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }
    
    Result RefreshLocationResolver(ncm::StorageId storage_id) {
        std::scoped_lock lk(g_mutex);
        auto resolver = g_location_resolvers.Find(storage_id);

        R_UNLESS(resolver, ResultUnknownStorageId());

        if (storage_id != ncm::StorageId::Host) {
            (*resolver)->Refresh();
        }

        return ResultSuccess();
    }

    Result OpenAddOnContentLocationResolver(sf::Out<std::shared_ptr<AddOnContentLocationResolverInterface>> out) {
        std::scoped_lock lk(g_mutex);

        if (!g_add_on_content_location_resolver) {
            g_add_on_content_location_resolver = std::make_shared<AddOnContentLocationResolverInterface>();
        }
        
        std::shared_ptr<AddOnContentLocationResolverInterface> new_intf = g_add_on_content_location_resolver;
        out.SetValue(std::move(new_intf));
        return ResultSuccess();
    }

}
