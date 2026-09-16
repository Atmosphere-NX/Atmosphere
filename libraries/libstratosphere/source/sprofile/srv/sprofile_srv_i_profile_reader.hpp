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
    AMS_SF_METHOD_INFO(C, H, 0,  Result, GetInt64Value,       (sf::Out<s64> out, sprofile::HashKey profile, sprofile::HashKey key),                                         (out, profile, key))                                  \
    AMS_SF_METHOD_INFO(C, H, 1,  Result, GetUInt64Value,      (sf::Out<u64> out, sprofile::HashKey profile, sprofile::HashKey key),                                         (out, profile, key))                                  \
    AMS_SF_METHOD_INFO(C, H, 2,  Result, GetInt32Value,       (sf::Out<s32> out, sprofile::HashKey profile, sprofile::HashKey key),                                         (out, profile, key))                                  \
    AMS_SF_METHOD_INFO(C, H, 3,  Result, GetUInt32Value,      (sf::Out<u32> out, sprofile::HashKey profile, sprofile::HashKey key),                                         (out, profile, key))                                  \
    AMS_SF_METHOD_INFO(C, H, 4,  Result, GetBooleanValue,     (sf::Out<u8> out, sprofile::HashKey profile, sprofile::HashKey key),                                          (out, profile, key))                                  \
    AMS_SF_METHOD_INFO(C, H, 5,  Result, GetAnyValue,         (sf::Out<u64> out, sprofile::HashKey profile, sprofile::HashKey key),                                         (out, profile, key),             hos::Version_23_0_0) \
    AMS_SF_METHOD_INFO(C, H, 6,  Result, GetInt8ValueArray,   (sf::Out<u32> out, const sf::OutBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key),        (out, out_buffer, profile, key), hos::Version_23_0_0) \
    AMS_SF_METHOD_INFO(C, H, 7,  Result, GetInt64ValueArray,  (sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key), (out, out_buffer, profile, key), hos::Version_23_0_0) \
    AMS_SF_METHOD_INFO(C, H, 8,  Result, GetUInt64ValueArray, (sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key), (out, out_buffer, profile, key), hos::Version_23_0_0) \
    AMS_SF_METHOD_INFO(C, H, 9,  Result, GetInt32ValueArray,  (sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key), (out, out_buffer, profile, key), hos::Version_23_0_0) \
    AMS_SF_METHOD_INFO(C, H, 10, Result, GetUInt32ValueArray, (sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key), (out, out_buffer, profile, key), hos::Version_23_0_0) \
    AMS_SF_METHOD_INFO(C, H, 11, Result, GetUInt8ValueArray,  (sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key), (out, out_buffer, profile, key), hos::Version_23_0_0) \
    AMS_SF_METHOD_INFO(C, H, 12, Result, GetAnyValueArray,    (sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key), (out, out_buffer, profile, key), hos::Version_23_0_0) 

AMS_SF_DEFINE_INTERFACE(ams::sprofile::srv, IProfileReader, AMS_SPROFILE_I_PROFILE_READER_INTERFACE_INFO, 0x97090D4D)
