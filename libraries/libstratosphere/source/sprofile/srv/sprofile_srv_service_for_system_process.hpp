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
#include "sprofile_srv_i_service_for_system_process.hpp"

namespace ams::sprofile::srv {

    class ProfileManager;

    class ServiceForSystemProcess {
        private:
            MemoryResource *m_memory_resource;
            ProfileManager *m_profile_manager;
        public:
            constexpr ServiceForSystemProcess(MemoryResource *mr, ProfileManager *pm) : m_memory_resource(mr), m_profile_manager(pm) { /* ... */ }
        public:
            Result OpenProfileReader(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileReader>> out);
            Result OpenProfileUpdateObserver(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileUpdateObserver>> out);
            Result OpenProfileControllerForDebug(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileControllerForDebug>> out);
    };
    static_assert(sprofile::IsISprofileServiceForSystemProcess<ServiceForSystemProcess>);

}
