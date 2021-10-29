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
#include "sprofile_srv_i_service_for_bg_agent.hpp"

namespace ams::sprofile::srv {

    class ProfileManager;

    class ServiceForBgAgent {
        private:
            MemoryResource *m_memory_resource;
            ProfileManager *m_profile_manager;
        public:
            constexpr ServiceForBgAgent(MemoryResource *mr, ProfileManager *pm) : m_memory_resource(mr), m_profile_manager(pm) { /* ... */ }
        public:
            Result OpenProfileImporter(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileImporter>> out);
            Result GetImportableProfileUrls(sf::Out<u32> out_count, const sf::OutArray<sprofile::srv::ProfileUrl> &out, const sprofile::srv::ProfileMetadataForImportMetadata &arg);
            Result IsUpdateNeeded(sf::Out<bool> out, Identifier revision_key);
            Result Reset();
    };
    static_assert(sprofile::IsISprofileServiceForBgAgent<ServiceForBgAgent>);

}
