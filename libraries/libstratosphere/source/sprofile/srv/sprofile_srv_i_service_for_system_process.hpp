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
#include "sprofile_srv_i_profile_reader.hpp"
#include "sprofile_srv_i_profile_update_observer.hpp"
#include "sprofile_srv_i_profile_controller_for_debug.hpp"

#define AMS_SPROFILE_I_SPROFILE_SERVICE_FOR_SYSTEM_PROCESS_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 100,  Result, OpenProfileReader,             (sf::Out<sf::SharedPointer<::ams::sprofile::IProfileReader>> out),             (out)) \
    AMS_SF_METHOD_INFO(C, H, 101,  Result, OpenProfileUpdateObserver,     (sf::Out<sf::SharedPointer<::ams::sprofile::IProfileUpdateObserver>> out),     (out)) \
    AMS_SF_METHOD_INFO(C, H, 900,  Result, OpenProfileControllerForDebug, (sf::Out<sf::SharedPointer<::ams::sprofile::IProfileControllerForDebug>> out), (out))

AMS_SF_DEFINE_INTERFACE(ams::sprofile, ISprofileServiceForSystemProcess, AMS_SPROFILE_I_SPROFILE_SERVICE_FOR_SYSTEM_PROCESS_INTERFACE_INFO)
