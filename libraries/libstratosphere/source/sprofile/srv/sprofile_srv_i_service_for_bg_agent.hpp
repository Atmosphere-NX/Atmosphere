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
#include "sprofile_srv_i_profile_importer.hpp"

#define AMS_SPROFILE_I_SPROFILE_SERVICE_FOR_BG_AGENT_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 100,  Result, OpenProfileImporter,      (sf::Out<sf::SharedPointer<::ams::sprofile::IProfileImporter>> out),                                                                      (out)) \
    AMS_SF_METHOD_INFO(C, H, 200,  Result, GetImportableProfileUrls, (sf::Out<u32> out_count, const sf::OutArray<sprofile::srv::ProfileUrl> &out, const sprofile::srv::ProfileMetadataForImportMetadata &arg), (out_count, out, arg)) \
    AMS_SF_METHOD_INFO(C, H, 201,  Result, IsUpdateNeeded,           (sf::Out<bool> out, sprofile::Identifier revision_key),                                                                                   (out, revision_key)) \
    AMS_SF_METHOD_INFO(C, H, 2000, Result, Reset,                    (),                                                                                                                                       ())

AMS_SF_DEFINE_INTERFACE(ams::sprofile, ISprofileServiceForBgAgent, AMS_SPROFILE_I_SPROFILE_SERVICE_FOR_BG_AGENT_INTERFACE_INFO)
