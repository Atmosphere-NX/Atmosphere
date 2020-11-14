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
#include "lr_remote_location_resolver_impl.hpp"
#include "lr_remote_registered_location_resolver_impl.hpp"

namespace ams::lr {

    namespace {

        bool g_initialized;

    }

    void Initialize() {
        AMS_ASSERT(!g_initialized);
        R_ABORT_UNLESS(lrInitialize());
        g_initialized = true;
    }

    void Finalize() {
        AMS_ASSERT(g_initialized);
        lrExit();
        g_initialized = false;
    }


    Result OpenLocationResolver(LocationResolver *out, ncm::StorageId storage_id) {
        LrLocationResolver lr;
        R_TRY(lrOpenLocationResolver(static_cast<NcmStorageId>(storage_id), std::addressof(lr)));

        *out = LocationResolver(sf::MakeShared<ILocationResolver, RemoteLocationResolverImpl>(lr));
        return ResultSuccess();
    }

    Result OpenRegisteredLocationResolver(RegisteredLocationResolver *out) {
        LrRegisteredLocationResolver lr;
        R_TRY(lrOpenRegisteredLocationResolver(std::addressof(lr)));

        *out = RegisteredLocationResolver(sf::MakeShared<IRegisteredLocationResolver, RemoteRegisteredLocationResolverImpl>(lr));
        return ResultSuccess();
    }

    Result OpenAddOnContentLocationResolver(AddOnContentLocationResolver *out) {
        /* TODO: libnx binding */
        AMS_ABORT();
    }

    Result RefreshLocationResolver(ncm::StorageId storage_id) {
        /* TODO: libnx binding */
        AMS_ABORT();
    }
}
