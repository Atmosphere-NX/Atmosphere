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
#include <stratosphere.hpp>
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_profile_controller_for_debug_impl.hpp"

namespace ams::sprofile::srv {

    Result ProfileControllerForDebugImpl::ClearSaveData() {
        R_RETURN(m_manager->ResetSaveData());
    }

    Result ProfileControllerForDebugImpl::QueryValue(sf::Out<u8> out_type, sf::Out<u64> out_value, sprofile::HashKey profile, sprofile::HashKey key) {
        R_RETURN(m_manager->QueryValue(out_type.GetPointer(), out_value.GetPointer(), profile, key));
    }
    
    Result ProfileControllerForDebugImpl::QueryValueArray(sf::Out<u8> out_type, sf::Out<u32> out_count, const sf::OutBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out_type, out_count, out_buffer, profile, key);
        R_SUCCEED();
    }

}
