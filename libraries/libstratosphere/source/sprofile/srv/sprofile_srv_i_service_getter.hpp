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
#include "sprofile_srv_i_service_for_system_process.hpp"
#include "sprofile_srv_i_service_for_bg_agent.hpp"

#define AMS_SPROFILE_I_SERVICE_GETTER_INTERFACE_INFO(C, H)                                                                                                    \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetServiceForSystemProcess, (sf::Out<sf::SharedPointer<sprofile::srv::ISprofileServiceForSystemProcess>> out), (out)) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, GetServiceForBgAgent,       (sf::Out<sf::SharedPointer<sprofile::srv::ISprofileServiceForBgAgent>> out), (out))

AMS_SF_DEFINE_INTERFACE(ams::sprofile::srv, IServiceGetter, AMS_SPROFILE_I_SERVICE_GETTER_INTERFACE_INFO, 0x2CFB8417)