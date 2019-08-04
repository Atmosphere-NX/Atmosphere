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
#include <inttypes.h>

#include "impl/lr_manager.hpp"
#include "lr_manager_service.hpp"
#include "debug.hpp"

namespace sts::lr {

    Result LocationResolverManagerService::OpenLocationResolver(Out<std::shared_ptr<ILocationResolver>> out, ncm::StorageId storage_id) {
        R_DEBUG_START
        return impl::OpenLocationResolver(out, storage_id);
        R_DEBUG_END
    }

    Result LocationResolverManagerService::OpenRegisteredLocationResolver(Out<std::shared_ptr<RegisteredLocationResolverInterface>> out) {
        R_DEBUG_START
        return impl::OpenRegisteredLocationResolver(out);
        R_DEBUG_END
    }

    Result LocationResolverManagerService::RefreshLocationResolver(ncm::StorageId storage_id) {
        R_DEBUG_START
        D_LOG("storage_id: 0x%x\n", static_cast<u8>(storage_id));
        return impl::RefreshLocationResolver(storage_id);
        R_DEBUG_END
    }

    Result LocationResolverManagerService::OpenAddOnContentLocationResolver(Out<std::shared_ptr<AddOnContentLocationResolverInterface>> out) {
        R_DEBUG_START
        return impl::OpenAddOnContentLocationResolver(out);
        R_DEBUG_END
    }

}