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
    AMS_SF_METHOD_INFO(C, H, 2000, Result, Reset,  (),                                                                                                     ())                                 \
    AMS_SF_METHOD_INFO(C, H, 2001, Result, GetRaw, (sf::Out<u8> out_type, sf::Out<u64> out_value, sprofile::Identifier profile, sprofile::Identifier key), (out_type, out_value, profile, key))


AMS_SF_DEFINE_INTERFACE(ams::sprofile, IProfileControllerForDebug, AMS_SPROFILE_I_PROFILE_CONTROLLER_FOR_DEBUG_INTERFACE_INFO)
