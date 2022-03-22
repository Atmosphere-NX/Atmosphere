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
#include <stratosphere/fs/fs_directory.hpp>

/* NOTE: Minimum firmware version not enforced for any commands. */
#define AMS_TMA_I_DIRECTORY_ACCESSOR_INTERFACE_INFO(C, H)                                                                                                                       \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetEntryCount,           (ams::sf::Out<s64> out),                                                                   (out))              \
    AMS_SF_METHOD_INFO(C, H, 1, Result, Read,                    (ams::sf::Out<s64> out, const ams::sf::OutMapAliasArray<fs::DirectoryEntry> &out_entries), (out, out_entries)) \
    AMS_SF_METHOD_INFO(C, H, 2, Result, SetPriorityForDirectory, (s32 priority),                                                                            (priority))         \
    AMS_SF_METHOD_INFO(C, H, 3, Result, GetPriorityForDirectory, (ams::sf::Out<s32> out),                                                                   (out))

AMS_SF_DEFINE_INTERFACE(ams::tma, IDirectoryAccessor, AMS_TMA_I_DIRECTORY_ACCESSOR_INTERFACE_INFO)
