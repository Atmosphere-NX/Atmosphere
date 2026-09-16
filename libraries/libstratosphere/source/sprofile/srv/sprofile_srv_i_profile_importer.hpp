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
#include "sprofile_srv_types.hpp"

#define AMS_SPROFILE_I_PROFILE_IMPORTER_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, ImportProfile,  (const sprofile::srv::ProfilePayload &import),  (import))                                           \
    AMS_SF_METHOD_INFO(C, H, 1, Result, Commit,         (),                                             (),       hos::Version_13_0_0, hos::Version_22_5_0) \
    AMS_SF_METHOD_INFO(C, H, 2, Result, ImportMetadata, (const sprofile::srv::MetadataPayload &import), (import), hos::Version_13_0_0, hos::Version_22_5_0)

AMS_SF_DEFINE_INTERFACE(ams::sprofile::srv, IProfileImporter, AMS_SPROFILE_I_PROFILE_IMPORTER_INTERFACE_INFO, 0x1629C4E6)
