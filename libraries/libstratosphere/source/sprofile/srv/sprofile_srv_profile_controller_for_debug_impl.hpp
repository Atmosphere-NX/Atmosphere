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
#include "sprofile_srv_i_profile_controller_for_debug.hpp"

namespace ams::sprofile::srv {

    class ProfileManager;

    class ProfileControllerForDebugImpl {
        private:
            ProfileManager *m_manager;
        public:
            ProfileControllerForDebugImpl(ProfileManager *manager) : m_manager(manager) { /* ... */ }
        public:
            Result ClearSaveData();
            Result QueryValue(sf::Out<u8> out_type, sf::Out<u64> out_value, sprofile::HashKey profile, sprofile::HashKey key);
            Result QueryValueArray(sf::Out<u8> out_type, sf::Out<u32> out_count, const sf::OutBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key);
    };
    static_assert(IsIProfileControllerForDebug<ProfileControllerForDebugImpl>);

}
