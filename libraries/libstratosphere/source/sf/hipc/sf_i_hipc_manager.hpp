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
#pragma once

#define AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO(C, H)                                                                                                   \
    AMS_SF_METHOD_INFO(C, H, 0, Result, ConvertCurrentObjectToDomain, (ams::sf::Out<ams::sf::cmif::DomainObjectId> out),                     (out))            \
    AMS_SF_METHOD_INFO(C, H, 1, Result, CopyFromCurrentDomain,        (ams::sf::OutMoveHandle out, ams::sf::cmif::DomainObjectId object_id), (out, object_id)) \
    AMS_SF_METHOD_INFO(C, H, 2, Result, CloneCurrentObject,           (ams::sf::OutMoveHandle out),                                          (out))            \
    AMS_SF_METHOD_INFO(C, H, 3, void,   QueryPointerBufferSize,       (ams::sf::Out<u16> out),                                               (out))            \
    AMS_SF_METHOD_INFO(C, H, 4, Result, CloneCurrentObjectEx,         (ams::sf::OutMoveHandle out, u32 tag),                                 (out, tag))

AMS_SF_DEFINE_INTERFACE(ams::sf::hipc::impl, IHipcManager, AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO, 0xEC6BE3FF)
