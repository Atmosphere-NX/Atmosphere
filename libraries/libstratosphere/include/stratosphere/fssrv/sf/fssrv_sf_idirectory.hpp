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
#include <stratosphere/fs/fs_directory.hpp>

namespace ams::fssrv::sf {

    #define AMS_FSSRV_I_DIRECTORY_INTERFACE_INFO(C, H)                                                                     \
        AMS_SF_METHOD_INFO(C, H, 0, Result, Read,          (ams::sf::Out<s64> out, const ams::sf::OutBuffer &out_entries)) \
        AMS_SF_METHOD_INFO(C, H, 1, Result, GetEntryCount, (ams::sf::Out<s64> out))

    AMS_SF_DEFINE_INTERFACE(IDirectory, AMS_FSSRV_I_DIRECTORY_INTERFACE_INFO)

}
