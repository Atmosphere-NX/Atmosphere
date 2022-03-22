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

/* NOTE: Minimum firmware version not enforced for any commands. */
#define AMS_TMA_I_FILE_ACCESSOR_INTERFACE_INFO(C, H)                                                                                                                                                   \
    AMS_SF_METHOD_INFO(C, H, 0, Result, ReadFile,           (ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, ams::fs::ReadOption option), (out, offset, buffer, option)) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, WriteFile,          (s64 offset, const ams::sf::InNonSecureBuffer &buffer, ams::fs::WriteOption option),                        (offset, buffer, option))      \
    AMS_SF_METHOD_INFO(C, H, 2, Result, GetFileSize,        (ams::sf::Out<s64> out),                                                                                    (out))                         \
    AMS_SF_METHOD_INFO(C, H, 3, Result, SetFileSize,        (s64 size),                                                                                                 (size))                        \
    AMS_SF_METHOD_INFO(C, H, 4, Result, FlushFile,          (),                                                                                                         ())                            \
    AMS_SF_METHOD_INFO(C, H, 5, Result, SetPriorityForFile, (s32 priority),                                                                                             (priority))                    \
    AMS_SF_METHOD_INFO(C, H, 6, Result, GetPriorityForFile, (ams::sf::Out<s32> out),                                                                                    (out))

AMS_SF_DEFINE_INTERFACE(ams::tma, IFileAccessor, AMS_TMA_I_FILE_ACCESSOR_INTERFACE_INFO)
