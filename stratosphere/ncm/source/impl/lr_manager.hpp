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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "../lr_addoncontentlocationresolver.hpp"
#include "../lr_ilocationresolver.hpp"
#include "../lr_registeredlocationresolver.hpp"

namespace sts::lr::impl {

    /* Location Resolver API. */
    Result OpenLocationResolver(Out<std::shared_ptr<ILocationResolver>> out, ncm::StorageId storage_id);
    Result OpenRegisteredLocationResolver(Out<std::shared_ptr<RegisteredLocationResolverInterface>> out);
    Result RefreshLocationResolver(ncm::StorageId storage_id);
    Result OpenAddOnContentLocationResolver(Out<std::shared_ptr<AddOnContentLocationResolverInterface>> out);

}
