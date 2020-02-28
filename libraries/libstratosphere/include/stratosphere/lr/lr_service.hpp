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
#include <stratosphere/lr/lr_location_resolver.hpp>
#include <stratosphere/lr/lr_add_on_content_location_resolver.hpp>
#include <stratosphere/lr/lr_registered_location_resolver.hpp>

namespace ams::lr {

    /* Management. */
    void Initialize();
    void Finalize();

    /* Service API. */
    Result OpenLocationResolver(LocationResolver *out, ncm::StorageId storage_id);
    Result OpenRegisteredLocationResolver(RegisteredLocationResolver *out);
    Result OpenAddOnContentLocationResolver(AddOnContentLocationResolver *out);
    Result RefreshLocationResolver(ncm::StorageId storage_id);

}
