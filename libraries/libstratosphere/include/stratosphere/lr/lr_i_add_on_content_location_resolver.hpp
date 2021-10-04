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
#include <stratosphere/lr/lr_types.hpp>

#define AMS_LR_I_ADD_ON_CONTENT_LOCATION_RESOLVER_INTERFACE_INFO(C, H)                                                                                                                                                                       \
    AMS_SF_METHOD_INFO(C, H, 0, Result, ResolveAddOnContentPath,               (sf::Out<lr::Path> out, ncm::DataId id),                                            (out, id),                        hos::Version_2_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 1, Result, RegisterAddOnContentStorageDeprecated, (ncm::DataId id, ncm::StorageId storage_id),                                        (id, storage_id),                 hos::Version_2_0_0, hos::Version_8_1_1) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, RegisterAddOnContentStorage,           (ncm::DataId id, ncm::ApplicationId application_id, ncm::StorageId storage_id),     (id, application_id, storage_id), hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 2, Result, UnregisterAllAddOnContentPath,         (),                                                                                 (),                               hos::Version_2_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 3, Result, RefreshApplicationAddOnContent,        (const sf::InArray<ncm::ApplicationId> &ids),                                       (ids),                            hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 4, Result, UnregisterApplicationAddOnContent,     (ncm::ApplicationId id),                                                            (id),                             hos::Version_9_0_0)

AMS_SF_DEFINE_INTERFACE(ams::lr, IAddOnContentLocationResolver, AMS_LR_I_ADD_ON_CONTENT_LOCATION_RESOLVER_INTERFACE_INFO)
