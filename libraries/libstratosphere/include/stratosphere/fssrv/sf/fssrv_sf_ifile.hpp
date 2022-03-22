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
#include <vapours.hpp>
#include <stratosphere/sf.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_query_range.hpp>

#define AMS_FSSRV_I_FILE_INTERFACE_INFO(C, H)                                                                                                                                                                                                        \
    AMS_SF_METHOD_INFO(C, H, 0, Result, Read,                   (ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size, ams::fs::ReadOption option),    (out, offset, buffer, size, option))                        \
    AMS_SF_METHOD_INFO(C, H, 1, Result, Write,                  (s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size, ams::fs::WriteOption option),                           (offset, buffer, size, option))                             \
    AMS_SF_METHOD_INFO(C, H, 2, Result, Flush,                  (),                                                                                                                      ())                                                         \
    AMS_SF_METHOD_INFO(C, H, 3, Result, SetSize,                (s64 size),                                                                                                              (size))                                                     \
    AMS_SF_METHOD_INFO(C, H, 4, Result, GetSize,                (ams::sf::Out<s64> out),                                                                                                 (out))                                                      \
    AMS_SF_METHOD_INFO(C, H, 5, Result, OperateRange,           (ams::sf::Out<ams::fs::FileQueryRangeInfo> out, s32 op_id, s64 offset, s64 size),                                        (out, op_id, offset, size),             hos::Version_4_0_0) \
    AMS_SF_METHOD_INFO(C, H, 6, Result, OperateRangeWithBuffer, (const ams::sf::OutNonSecureBuffer &out_buf, const ams::sf::InNonSecureBuffer &in_buf, s32 op_id, s64 offset, s64 size), (out_buf, in_buf, op_id, offset, size), hos::Version_12_0_0)

AMS_SF_DEFINE_INTERFACE(ams::fssrv::sf, IFile, AMS_FSSRV_I_FILE_INTERFACE_INFO)
