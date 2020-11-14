/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours.hpp>
#include <stratosphere/sf.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_query_range.hpp>

namespace ams::fssrv::sf {

    #define AMS_FSSRV_I_STORAGE_INTERFACE_INFO(C, H)                                                                                                               \
        AMS_SF_METHOD_INFO(C, H, 0, Result, Read,         (s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size))                                       \
        AMS_SF_METHOD_INFO(C, H, 1, Result, Write,        (s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size))                                        \
        AMS_SF_METHOD_INFO(C, H, 2, Result, Flush,        ())                                                                                                      \
        AMS_SF_METHOD_INFO(C, H, 3, Result, SetSize,      (s64 size))                                                                                              \
        AMS_SF_METHOD_INFO(C, H, 4, Result, GetSize,      (ams::sf::Out<s64> out))                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 5, Result, OperateRange, (ams::sf::Out<ams::fs::StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size), hos::Version_4_0_0)

    AMS_SF_DEFINE_INTERFACE(IStorage, AMS_FSSRV_I_STORAGE_INTERFACE_INFO)

}
