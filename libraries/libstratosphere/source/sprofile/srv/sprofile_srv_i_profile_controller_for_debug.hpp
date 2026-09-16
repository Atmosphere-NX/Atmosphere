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

#define AMS_SPROFILE_I_PROFILE_CONTROLLER_FOR_DEBUG_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 2000, Result, ClearSaveData,   (),                                                                                                                                ())                                                                   \
    AMS_SF_METHOD_INFO(C, H, 2001, Result, QueryValue,      (sf::Out<u8> out_type, sf::Out<u64> out_value, sprofile::HashKey profile, sprofile::HashKey key),                                  (out_type, out_value, profile, key))                                  \
    AMS_SF_METHOD_INFO(C, H, 2002, Result, QueryValueArray, (sf::Out<u8> out_type, sf::Out<u32> out_count, const sf::OutBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key), (out_type, out_count, out_buffer, profile, key), hos::Version_23_0_0)

AMS_SF_DEFINE_INTERFACE(ams::sprofile::srv, IProfileControllerForDebug, AMS_SPROFILE_I_PROFILE_CONTROLLER_FOR_DEBUG_INTERFACE_INFO, 0xA8C14F64)
