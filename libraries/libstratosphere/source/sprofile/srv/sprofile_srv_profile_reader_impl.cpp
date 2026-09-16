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
#include "sprofile_srv_profile_reader_impl.hpp"

namespace ams::sprofile::srv {

    Result ProfileReaderImpl::GetInt64Value(sf::Out<s64> out, sprofile::HashKey profile, sprofile::HashKey key) {
        R_RETURN(m_manager->GetInt64Value(out.GetPointer(), profile, key));
    }

    Result ProfileReaderImpl::GetUInt64Value(sf::Out<u64> out, sprofile::HashKey profile, sprofile::HashKey key) {
        R_RETURN(m_manager->GetUInt64Value(out.GetPointer(), profile, key));
    }

    Result ProfileReaderImpl::GetInt32Value(sf::Out<s32> out, sprofile::HashKey profile, sprofile::HashKey key) {
        R_RETURN(m_manager->GetInt32Value(out.GetPointer(), profile, key));
    }

    Result ProfileReaderImpl::GetUInt32Value(sf::Out<u32> out, sprofile::HashKey profile, sprofile::HashKey key) {
        R_RETURN(m_manager->GetUInt32Value(out.GetPointer(), profile, key));
    }

    Result ProfileReaderImpl::GetBooleanValue(sf::Out<u8> out, sprofile::HashKey profile, sprofile::HashKey key) {
        R_RETURN(m_manager->GetBooleanValue(out.GetPointer(), profile, key));
    }
    
    Result ProfileReaderImpl::GetAnyValue(sf::Out<u64> out, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, profile, key);
        R_SUCCEED();
    }
    
    Result ProfileReaderImpl::GetInt8ValueArray(sf::Out<u32> out, const sf::OutBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, out_buffer, profile, key);
        R_SUCCEED();
    }
    
    Result ProfileReaderImpl::GetInt64ValueArray(sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, out_buffer, profile, key);
        R_SUCCEED();
    }
    
    Result ProfileReaderImpl::GetUInt64ValueArray(sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, out_buffer, profile, key);
        R_SUCCEED();
    }
    
    Result ProfileReaderImpl::GetInt32ValueArray(sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, out_buffer, profile, key);
        R_SUCCEED();
    }
    
    Result ProfileReaderImpl::GetUInt32ValueArray(sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, out_buffer, profile, key);
        R_SUCCEED();
    }
    
    Result ProfileReaderImpl::GetUInt8ValueArray(sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, out_buffer, profile, key);
        R_SUCCEED();
    }
    
    Result ProfileReaderImpl::GetAnyValueArray(sf::Out<u32> out, const sf::OutPointerBuffer &out_buffer, sprofile::HashKey profile, sprofile::HashKey key) {
        /* On 23.0.0+ this isn't reachable from HIPC. */
        AMS_UNUSED(out, out_buffer, profile, key);
        R_SUCCEED();
    }

}
