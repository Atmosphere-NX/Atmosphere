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

#define AMS_SPROFILE_I_PROFILE_READER_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetSigned64,   (sf::Out<s64> out, sprofile::Identifier profile, sprofile::Identifier key), (out, profile, key)) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, GetUnsigned64, (sf::Out<u64> out, sprofile::Identifier profile, sprofile::Identifier key), (out, profile, key)) \
    AMS_SF_METHOD_INFO(C, H, 2, Result, GetSigned32,   (sf::Out<s32> out, sprofile::Identifier profile, sprofile::Identifier key), (out, profile, key)) \
    AMS_SF_METHOD_INFO(C, H, 3, Result, GetUnsigned32, (sf::Out<u32> out, sprofile::Identifier profile, sprofile::Identifier key), (out, profile, key)) \
    AMS_SF_METHOD_INFO(C, H, 4, Result, GetByte,       (sf::Out<u8> out, sprofile::Identifier profile, sprofile::Identifier key),  (out, profile, key))

AMS_SF_DEFINE_INTERFACE(ams::sprofile, IProfileReader, AMS_SPROFILE_I_PROFILE_READER_INTERFACE_INFO)
