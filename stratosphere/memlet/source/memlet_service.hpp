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
#include <stratosphere.hpp>

#define AMS_MEMLET_I_SERVICE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, CreateAppletSharedMemory, (sf::Out<u64> out_size, sf::OutMoveHandle out_handle, u64 desired_size), (out_size, out_handle, desired_size))

AMS_SF_DEFINE_INTERFACE(ams::memlet::impl, IService, AMS_MEMLET_I_SERVICE_INTERFACE_INFO, 0x00000000)

namespace ams::memlet {

    class Service {
        public:
            Result CreateAppletSharedMemory(sf::Out<u64> out_size, sf::OutMoveHandle out_handle, u64 desired_size);
    };
    static_assert(impl::IsIService<Service>);

}
